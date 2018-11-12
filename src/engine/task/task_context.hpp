#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <ev.h>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <engine/deadline.hpp>
#include <engine/task/task.hpp>
#include <engine/task/task_context_holder.hpp>
#include <utils/flags.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/wait_list.hpp>
#include "cxxabi_eh_globals.hpp"
#include "task_counter.hpp"

namespace engine {

class TaskProcessor;

namespace impl {

class TaskContext : public boost::intrusive_ref_counter<TaskContext> {
 public:
  using CoroutinePtr = coro::Pool<TaskContext>::CoroutinePtr;
  using TaskPipe = coro::Pool<TaskContext>::TaskPipe;
  using CoroId = uint64_t;
  using TaskId = uint64_t;

  enum class YieldReason { kNone, kTaskWaiting, kTaskCancelled, kTaskComplete };
  enum class WakeupSource {
    kNone,
    kCancelRequest,
    kWaitList,
    kDeadlineTimer,
  };

  struct SleepParams {
    // no deadline by default
    Deadline deadline;
    // list to cleanup in case of cancel
    std::weak_ptr<WaitListBase> wait_list;
    // called from the same thread after leaving coroutine
    Payload exec_after_asleep;
    // called from coroutine before leaving Sleep()
    Payload exec_before_awake;
  };

  TaskContext(TaskProcessor&, Task::Importance, Payload);

  ~TaskContext();

  TaskContext(const TaskContext&) = delete;
  TaskContext(TaskContext&&) = delete;
  TaskContext& operator=(const TaskContext&) = delete;
  TaskContext& operator=(TaskContext&&) = delete;

  Task::State GetState() const { return state_; }

  // whether task respects task processor queue size limits
  // exceeding these limits causes task to become cancelled
  bool IsCritical() const;

  // whether user code finished executing, coroutine may still be running
  bool IsFinished() const {
    return state_ == Task::State::kCompleted ||
           state_ == Task::State::kCancelled;
  }

  // for use by TaskProcessor only
  bool IsDetached() { return is_detached_; }
  void SetDetached();

  // wait for this to become finished
  // should only be called from other context
  void Wait() const;
  void WaitUntil(Deadline) const;

  TaskProcessor& GetTaskProcessor() { return task_processor_; }
  void DoStep();

  // normally non-blocking, causes wakeup
  void RequestCancel(Task::CancellationReason);
  Task::CancellationReason GetCancellationReason() const {
    return cancellation_reason_;
  }
  bool IsCancelRequested() {
    return cancellation_reason_ != Task::CancellationReason::kNone;
  }

  // for use by ~Task()
  // returns previous value
  bool SetCancellable(bool);

  // causes this to yield and wait for wakeup
  // must only be called from this context
  // "spurious wakeups" may be caused by wakeup queueing
  void Sleep(SleepParams&&);

  // causes this to return from the nearest sleep
  // i.e. wakeup is queued if task is running
  // normally non-blocking, except corner cases in TaskProcessor::Schedule()
  void Wakeup(WakeupSource);
  WakeupSource GetWakeupSource() const { return wakeup_source_; }

  static void CoroFunc(TaskPipe& task_pipe);

  // C++ ABI support, not to be used by anyone
  EhGlobals* GetEhGlobals() { return &eh_globals_; }

  TaskId GetTaskId() const { return reinterpret_cast<uint64_t>(this); }

  CoroId GetCoroId() const { return reinterpret_cast<CoroId>(coro_.get()); }

  std::chrono::steady_clock::time_point GetQueueWaitTimepoint() const {
    return task_queue_wait_timepoint_;
  }

  void SetQueueWaitTimepoint(std::chrono::steady_clock::time_point tp) {
    task_queue_wait_timepoint_ = tp;
  }

 private:
  static constexpr uint64_t kMagic = 0x6b73615453755459ull;  // "YTuSTask"

  enum class SleepStateFlags {
    kNone = 0,
    kSleeping = (1 << 0),
    kWakeupRequested = (1 << 1),
  };

  void SetState(Task::State);

  void Schedule();
  void Unwind();

  void ProfilerStartExecution();

  void ProfilerStopExecution();

  const uint64_t magic_;
  TaskProcessor& task_processor_;
  const TaskCounter::Token task_counter_token_;
  const bool is_critical_;
  EhGlobals eh_globals_;
  Payload payload_;

  std::atomic<Task::State> state_;
  std::atomic<bool> is_detached_;
  std::atomic<bool> is_cancellable_;
  std::atomic<Task::CancellationReason> cancellation_reason_;
  std::shared_ptr<WaitList> finish_waiters_;

  // () if not defined
  std::chrono::steady_clock::time_point task_queue_wait_timepoint_;
#ifdef USERVER_PROFILER
  std::chrono::steady_clock::time_point execute_started_;
#endif  // USERVER_PROFILER

  SleepParams sleep_params_;
  utils::AtomicFlags<SleepStateFlags> sleep_state_;
  std::atomic<WakeupSource> wakeup_source_;

  CoroutinePtr coro_;
  TaskPipe* task_pipe_;
  YieldReason yield_reason_;
};

}  // namespace impl
namespace current_task {

impl::TaskContext* GetCurrentTaskContext();
impl::TaskContext* GetCurrentTaskContextUnchecked();

}  // namespace current_task
}  // namespace engine
