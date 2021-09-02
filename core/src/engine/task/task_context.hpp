#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

#include <ev.h>
#include <boost/intrusive/list_hook.hpp>
#include <boost/smart_ptr/intrusive_ref_counter.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/thread_control.hpp>
#include <engine/ev/timer.hpp>
#include <engine/task/counted_coroutine_ptr.hpp>
#include <engine/task/cxxabi_eh_globals.hpp>
#include <engine/task/sleep_state.hpp>
#include <engine/task/task_counter.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/local_storage.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_context_holder.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/utils/flags.hpp>

namespace engine {
namespace impl {

[[noreturn]] void ReportDeadlock();

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

 protected:
  constexpr WaitStrategy(Deadline deadline) noexcept : deadline_(deadline) {}

 private:
  const Deadline deadline_;
};

class TaskContext final : public boost::intrusive_ref_counter<TaskContext> {
 public:
  struct NoEpoch {};
  using TaskPipe = coro::Pool<TaskContext>::TaskPipe;
  using CoroId = uint64_t;
  using TaskId = uint64_t;

  enum class YieldReason { kNone, kTaskWaiting, kTaskCancelled, kTaskComplete };

  /// Wakeup sources in descending priority order
  enum class WakeupSource : uint32_t {
    kNone = static_cast<uint32_t>(SleepFlags::kNone),
    kWaitList = static_cast<uint32_t>(SleepFlags::kWakeupByWaitList),
    kDeadlineTimer = static_cast<uint32_t>(SleepFlags::kWakeupByDeadlineTimer),
    kCancelRequest = static_cast<uint32_t>(SleepFlags::kWakeupByCancelRequest),
    kBootstrap = static_cast<uint32_t>(SleepFlags::kWakeupByBootstrap),
  };

  TaskContext(TaskProcessor&, Task::Importance, Deadline, Payload);

  ~TaskContext() noexcept;

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
  void RequestCancel(TaskCancellationReason);

  TaskCancellationReason CancellationReason() const {
    return cancellation_reason_;
  }

  bool IsCancelRequested() {
    return cancellation_reason_ != TaskCancellationReason::kNone ||
           CheckDeadline();
  }

  bool IsCancellable() const noexcept;
  // returns previous value
  bool SetCancellable(bool);

  bool ShouldCancel() { return IsCancelRequested() && IsCancellable(); }

  // causes this to yield and wait for wakeup
  // must only be called from this context
  // "spurious wakeups" may be caused by wakeup queueing
  WakeupSource Sleep(WaitStrategy& wait_strategy);

  void ArmDeadlineTimer(Deadline deadline, SleepState::Epoch sleep_epoch);

  // causes this to return from the nearest sleep
  // i.e. wakeup is queued if task is running
  // normally non-blocking, except corner cases in TaskProcessor::Schedule()
  void Wakeup(WakeupSource, SleepState::Epoch epoch);
  void Wakeup(WakeupSource, NoEpoch);

  // Must be called from this
  WakeupSource DebugGetWakeupSource() const;

  static void CoroFunc(TaskPipe& task_pipe);

  // C++ ABI support, not to be used by anyone
  EhGlobals* GetEhGlobals() { return &eh_globals_; }

  TaskId GetTaskId() const { return reinterpret_cast<TaskId>(this); }

  CoroId GetCoroId() const { return coro_.Id(); }

  std::chrono::steady_clock::time_point GetQueueWaitTimepoint() const {
    return task_queue_wait_timepoint_;
  }

  void SetQueueWaitTimepoint(std::chrono::steady_clock::time_point tp) {
    task_queue_wait_timepoint_ = tp;
  }

  void SetCancelDeadline(Deadline deadline);

  bool HasLocalStorage() const;
  LocalStorage& GetLocalStorage();

 private:
  friend class Task::WaitAnyElement;

  static constexpr uint64_t kMagic = 0x6b73615453755459ULL;  // "YTuSTask"

  static WakeupSource GetPrimaryWakeupSource(SleepState::Flags sleep_flags);

  bool WasStartedAsCritical() const;
  void SetState(Task::State);

  void Schedule();
  static bool ShouldSchedule(SleepState::Flags flags, WakeupSource source);

  void ProfilerStartExecution();
  void ProfilerStopExecution();

  void TraceStateTransition(Task::State state);

  [[nodiscard]] auto UseWaitStrategy(WaitStrategy& wait_strategy) noexcept;

  bool CheckDeadline();

  [[maybe_unused]] const uint64_t magic_;
  TaskProcessor& task_processor_;
  TaskCounter::Token task_counter_token_;
  const bool is_critical_;
  EhGlobals eh_globals_;
  Payload payload_;

  std::atomic<Task::State> state_;
  std::atomic<bool> is_detached_;
  bool is_cancellable_;
  std::atomic<TaskCancellationReason> cancellation_reason_;
  mutable FastPimplWaitListLight finish_waiters_;

  ev::Timer sleep_deadline_timer_;

  engine::Deadline cancel_deadline_;

  // {} if not defined
  std::chrono::steady_clock::time_point task_queue_wait_timepoint_;
  std::chrono::steady_clock::time_point execute_started_;
  std::chrono::steady_clock::time_point last_state_change_timepoint_;

  size_t trace_csw_left_;

  WaitStrategy* wait_manager_;
  std::atomic<SleepState> sleep_state_;
  WakeupSource wakeup_source_;

  CountedCoroutinePtr coro_;
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

 public:
  using WaitListHook = typename boost::intrusive::make_list_member_hook<
      boost::intrusive::link_mode<boost::intrusive::auto_unlink>>::type;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  WaitListHook wait_list_hook;
};

}  // namespace impl

namespace current_task {

impl::TaskContext* GetCurrentTaskContext() noexcept;
impl::TaskContext* GetCurrentTaskContextUnchecked() noexcept;

}  // namespace current_task
}  // namespace engine
