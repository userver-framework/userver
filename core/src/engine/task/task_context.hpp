#pragma once

#include <atomic>
#include <memory>
#include <vector>

#include <ev.h>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <engine/coro/pool.hpp>
#include <engine/deadline.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/task/local_storage.hpp>
#include <engine/task/task.hpp>
#include <engine/task/task_context_holder.hpp>
#include <engine/wait_list_light.hpp>
#include <utils/flags.hpp>
#include "cxxabi_eh_globals.hpp"
#include "task_counter.hpp"

namespace engine {

class TaskProcessor;

namespace impl {

class WaitStrategy {
 public:
  // Implementation may setup timers/watchers here. Implementation must make
  // sure that there is no race between AfterAsleep() and WaitList-specific
  // wakeup (if "add task to wait list iff not ready" is not protected from
  // Wakeup, e.g. for WaitListLight). AfterAsleep() *may* call Wakeup() for
  // current task - sleep_state_ is set in DoStep() and double checked for such
  // early wakeups. It may not sleep.
  virtual void AfterAsleep() = 0;

  // Implementation should delete current task from all wait lists, stop all
  // timers/watchers, and release all allocated resources.
  // sleep_state_ will be cleared later at Sleep() return. Current task should
  // acquire all resources if needed (e.g. mutex), thus it may sleep.
  virtual void BeforeAwake() = 0;

  Deadline GetDeadline() const { return deadline_; }

  virtual std::shared_ptr<WaitListBase> GetWaitList() = 0;

 protected:
  WaitStrategy(Deadline deadline) : deadline_(deadline) {}

 private:
  const Deadline deadline_;
};

class TaskContext : public boost::intrusive_ref_counter<TaskContext> {
 public:
  using CoroutinePtr = coro::Pool<TaskContext>::CoroutinePtr;
  using TaskPipe = coro::Pool<TaskContext>::TaskPipe;
  using CoroId = uint64_t;
  using TaskId = uint64_t;

  enum class YieldReason { kNone, kTaskWaiting, kTaskCancelled, kTaskComplete };

  /// Wakeup sources in descending priority order
  enum class WakeupSource {
    kNone = 0,
    kWaitList = (1 << 0),
    kDeadlineTimer = (1 << 1),
    kCancelRequest = (1 << 2),
    kBootstrap = (1 << 3),
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
  bool IsCancelRequested() const {
    return cancellation_reason_ != Task::CancellationReason::kNone;
  }

  bool IsCancellable() const;
  // returns previous value
  bool SetCancellable(bool);

  bool ShouldCancel() const { return IsCancelRequested() && IsCancellable(); }

  // causes this to yield and wait for wakeup
  // must only be called from this context
  // "spurious wakeups" may be caused by wakeup queueing
  void Sleep(WaitStrategy*);

  // causes this to return from the nearest sleep
  // i.e. wakeup is queued if task is running
  // normally non-blocking, except corner cases in TaskProcessor::Schedule()
  void Wakeup(WakeupSource);

  // Must be called from this
  WakeupSource GetWakeupSource() const;

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

  bool HasLocalStorage() const;
  LocalStorage& GetLocalStorage();

 private:
  static constexpr uint64_t kMagic = 0x6b73615453755459ull;  // "YTuSTask"

  enum class SleepStateFlags {
    kNone = 0,
    kWakeupByWaitList = static_cast<int>(WakeupSource::kWaitList),
    kWakeupByDeadlineTimer = static_cast<int>(WakeupSource::kDeadlineTimer),
    kWakeupByCancelRequest = static_cast<int>(WakeupSource::kCancelRequest),
    kWakeupByBootstrap = static_cast<int>(WakeupSource::kBootstrap),

    kSleeping = (kWakeupByBootstrap << 1),
    kNonCancellable = (kSleeping << 1)
  };
  static WakeupSource GetPrimaryWakeupSource(
      utils::Flags<SleepStateFlags> sleep_state);

  void SetState(Task::State);

  void Schedule();
  static bool ShouldSchedule(utils::Flags<SleepStateFlags> flags,
                             WakeupSource source);

  void ProfilerStartExecution();
  void ProfilerStopExecution();

  void TraceStateTransition(Task::State state);

 private:
  [[maybe_unused]] const uint64_t magic_;
  TaskProcessor& task_processor_;
  const TaskCounter::Token task_counter_token_;
  const bool is_critical_;
  EhGlobals eh_globals_;
  Payload payload_;

  std::atomic<Task::State> state_;
  std::atomic<bool> is_detached_;
  bool is_cancellable_;
  std::atomic<Task::CancellationReason> cancellation_reason_;
  std::shared_ptr<WaitListLight> finish_waiters_;

  // () if not defined
  std::chrono::steady_clock::time_point task_queue_wait_timepoint_;
#ifdef USERVER_PROFILER
  std::chrono::steady_clock::time_point execute_started_;
#endif  // USERVER_PROFILER
  std::chrono::steady_clock::time_point last_state_change_timepoint_;

  size_t trace_csw_left_;

  WaitStrategy* wait_manager_;
  utils::AtomicFlags<SleepStateFlags> sleep_state_;
  WakeupSource wakeup_source_;

  CoroutinePtr coro_;
  TaskPipe* task_pipe_;
  YieldReason yield_reason_;

  class LocalStorageGuard {
   public:
    LocalStorageGuard(TaskContext& context);

    ~LocalStorageGuard();

   private:
    TaskContext& context_;
    LocalStorage local_storage_;
  };

  LocalStorage* local_storage_;
};

}  // namespace impl
namespace current_task {

impl::TaskContext* GetCurrentTaskContext();
impl::TaskContext* GetCurrentTaskContextUnchecked();

}  // namespace current_task
}  // namespace engine
