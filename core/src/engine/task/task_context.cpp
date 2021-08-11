#include "task_context.hpp"

#include <exception>

#include <fmt/format.h>
#include <boost/stacktrace.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/timer.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/underlying_value.hpp>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/coro_unwinder.hpp>
#include <engine/task/cxxabi_eh_globals.hpp>
#include <engine/task/task_processor.hpp>

namespace engine {
namespace current_task {
namespace {

thread_local impl::TaskContext* current_task_context_ptr = nullptr;

void SetCurrentTaskContext(impl::TaskContext* context) {
  UASSERT(!current_task_context_ptr || !context);
  current_task_context_ptr = context;
}

// This MUST be a separate function! Putting the body of this function into
// GetCurrentTaskContext() clobbers too many registers and compiler decides to
// use stack memory in GetCurrentTaskContext(). This leads to slowdown
// of GetCurrentTaskContext(). In particular Mutex::lock() slows down on ~25%.
[[noreturn]] void ReportOutsideTheCoroutineCall() {
  UASSERT_MSG(false,
              "current_task::GetCurrentTaskContext() called outside coroutine");
  LOG_CRITICAL()
      << "current_task::GetCurrentTaskContext() called outside coroutine"
      << logging::LogExtra::Stacktrace();
  throw std::logic_error(
      "current_task::GetCurrentTaskContext() called outside coroutine. "
      "stacktrace:\n" +
      logging::stacktrace_cache::to_string(boost::stacktrace::stacktrace{}));
}

}  // namespace

impl::TaskContext* GetCurrentTaskContext() {
  if (!current_task_context_ptr) {
    ReportOutsideTheCoroutineCall();
  }
  return current_task_context_ptr;
}

impl::TaskContext* GetCurrentTaskContextUnchecked() {
  return current_task_context_ptr;
}

}  // namespace current_task

namespace impl {
namespace {

[[noreturn]] void ReportDeadlock() {
  UASSERT_MSG(false, "Coroutine attempted to wait for itself");

  LOG_CRITICAL() << "Coroutine attempted to wait for itself"
                 << logging::LogExtra::Stacktrace();
  throw std::logic_error(
      "Coroutine attempted to wait for itself. stacktrace:\n" +
      logging::stacktrace_cache::to_string(boost::stacktrace::stacktrace{}));
}

std::string GetTaskIdString(const TaskContext* task) {
  return fmt::format("{:X}", task ? task->GetTaskId() : 0);
}

class CurrentTaskScope final {
 public:
  explicit CurrentTaskScope(TaskContext& context, EhGlobals& eh_store)
      : eh_store_(eh_store) {
    current_task::SetCurrentTaskContext(&context);
    ExchangeEhGlobals(eh_store_);
  }

  ~CurrentTaskScope() {
    ExchangeEhGlobals(eh_store_);
    current_task::SetCurrentTaskContext(nullptr);
  }

 private:
  EhGlobals& eh_store_;
};

template <typename Func>
void CallOnce(Func& func) {
  if (func) {
    auto func_to_destroy = std::move(func);
    func_to_destroy();
  }
}

// SleepState should be efficient, see https://godbolt.org/z/WWcsd9
static_assert(std::atomic<SleepState>::is_always_lock_free);

SleepState FetchOr(std::atomic<SleepState>& sleep_state,
                   SleepState::Flags flags,
                   std::memory_order success = std::memory_order_seq_cst) {
  auto prev_sleep_state = sleep_state.load(std::memory_order_relaxed);
  do {
    auto new_state = prev_sleep_state;
    new_state.flags |= flags;

    if (sleep_state.compare_exchange_weak(prev_sleep_state, new_state, success,
                                          std::memory_order_relaxed)) {
      return prev_sleep_state;
    }
  } while (true);
}

constexpr SleepState MakeNextEpochSleepState(SleepState::Epoch current) {
  using Epoch = SleepState::Epoch;
  return {Epoch{utils::UnderlyingValue(current) + 1}, SleepFlags::kNone};
}

class NoopWaitStrategy final : public WaitStrategy {
 public:
  static NoopWaitStrategy& Instance() noexcept {
    static /*constinit*/ NoopWaitStrategy instance;
    return instance;
  }

  void AfterAsleep() override {}

  void BeforeAwake() override {}

 private:
  constexpr NoopWaitStrategy() noexcept : WaitStrategy(Deadline{}) {}
};

}  // namespace

TaskContext::LocalStorageGuard::LocalStorageGuard(TaskContext& context)
    : context_(context) {
  context_.local_storage_ = &local_storage_;
}

TaskContext::LocalStorageGuard::~LocalStorageGuard() {
  context_.local_storage_ = nullptr;
}

TaskContext::TaskContext(TaskProcessor& task_processor,
                         Task::Importance importance, Deadline deadline,
                         Payload payload)
    : magic_(kMagic),
      task_processor_(task_processor),
      task_counter_token_(task_processor_.GetTaskCounter()),
      is_critical_(importance == Task::Importance::kCritical),
      payload_(std::move(payload)),
      state_(Task::State::kNew),
      is_detached_(false),
      is_cancellable_(true),
      cancellation_reason_(TaskCancellationReason::kNone),
      finish_waiters_(),
      cancel_deadline_(deadline),
      trace_csw_left_(task_processor_.GetTaskTraceMaxCswForNewTask()),
      wait_manager_(&NoopWaitStrategy::Instance()),
      sleep_state_(SleepState{SleepState::Epoch{0}, SleepFlags::kSleeping}),
      wakeup_source_(WakeupSource::kNone),
      task_pipe_(nullptr),
      yield_reason_(YieldReason::kNone),
      local_storage_(nullptr) {
  UASSERT(payload_);
  LOG_TRACE() << "task with task_id="
              << GetTaskIdString(current_task::GetCurrentTaskContextUnchecked())
              << " created task with task_id=" << GetTaskIdString(this)
              << logging::LogExtra::Stacktrace();
}

TaskContext::~TaskContext() noexcept {
  LOG_TRACE() << "Task with task_id=" << GetTaskIdString(this) << " stopped"
              << logging::LogExtra::Stacktrace();
}

bool TaskContext::IsCritical() const {
  // running tasks must not be susceptible to overload
  // e.g. we might need to run coroutine to cancel it
  return WasStartedAsCritical() || coro_;
}

void TaskContext::SetDetached() {
  [[maybe_unused]] bool was_detached = is_detached_.exchange(true);
  UASSERT(!was_detached);
}

void TaskContext::Wait() const { WaitUntil({}); }

namespace impl {

class LockedWaitStrategy final : public WaitStrategy {
 public:
  LockedWaitStrategy(Deadline deadline, WaitListLight& waiters,
                     TaskContext& current, const TaskContext& target)
      : WaitStrategy(deadline),
        waiters_(waiters),
        current_(current),
        target_(target) {}

  void AfterAsleep() override {
    waiters_.Append(&current_);
    if (target_.IsFinished()) waiters_.WakeupOne();
  }

  void BeforeAwake() override { waiters_.Remove(&current_); }

 private:
  WaitListLight& waiters_;
  TaskContext& current_;
  const TaskContext& target_;
};

};  // namespace impl

void TaskContext::WaitUntil(Deadline deadline) const {
  // try to avoid ctx switch if possible
  if (IsFinished()) return;

  auto current = current_task::GetCurrentTaskContext();
  if (current == this) ReportDeadlock();

  if (current->ShouldCancel()) {
    throw WaitInterruptedException(current->cancellation_reason_);
  }

  impl::LockedWaitStrategy wait_manager(deadline, *finish_waiters_, *current,
                                        *this);
  current->Sleep(wait_manager);

  if (!IsFinished() && current->ShouldCancel()) {
    throw WaitInterruptedException(current->cancellation_reason_);
  }
}

void TaskContext::DoStep() {
  if (IsFinished()) return;

  SleepState::Flags clear_flags{SleepFlags::kSleeping};
  if (!coro_) {
    // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
    coro_ = task_processor_.GetCoroutine();
    clear_flags |= SleepFlags::kWakeupByBootstrap;
  }
  // Do non-atomic fetch_and() - we don't care about lost spurious
  // wakeup events
  auto new_sleep_state = sleep_state_.load();
  new_sleep_state.flags.Clear(clear_flags);
  sleep_state_.store(new_sleep_state, std::memory_order_relaxed);

  // eh_globals is replaced in task scope, we must proxy the exception
  std::exception_ptr uncaught;
  {
    CurrentTaskScope current_task_scope(*this, eh_globals_);
    try {
      SetState(Task::State::kRunning);
      (*coro_)(this);
      UASSERT(wait_manager_);
      wait_manager_->AfterAsleep();
    } catch (...) {
      uncaught = std::current_exception();
    }
  }
  if (uncaught) std::rethrow_exception(uncaught);

  switch (yield_reason_) {
    case YieldReason::kTaskCancelled:
    case YieldReason::kTaskComplete:
      // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.UndefReturn)
      std::move(coro_).ReturnToPool();
      {
        auto new_state = (yield_reason_ == YieldReason::kTaskComplete)
                             ? Task::State::kCompleted
                             : Task::State::kCancelled;
        SetState(new_state);
        TraceStateTransition(new_state);
      }
      break;

    case YieldReason::kTaskWaiting:
      SetState(Task::State::kSuspended);
      {
        SleepState::Flags new_flags = SleepFlags::kSleeping;
        if (!IsCancellable()) new_flags |= SleepFlags::kNonCancellable;

        // Synchronization point for relaxed SetState()
        auto prev_sleep_state = FetchOr(sleep_state_, new_flags);

        UASSERT(!(prev_sleep_state.flags & SleepFlags::kSleeping));
        if (new_flags & SleepFlags::kNonCancellable)
          prev_sleep_state.flags.Clear({SleepFlags::kWakeupByCancelRequest,
                                        SleepFlags::kNonCancellable});
        if (prev_sleep_state.flags) {
          Schedule();
        }
      }
      break;

    case YieldReason::kNone:
      UASSERT(!"invalid yield reason");
      throw std::logic_error("invalid yield reason");
  }
}

void TaskContext::RequestCancel(TaskCancellationReason reason) {
  auto expected = TaskCancellationReason::kNone;
  if (cancellation_reason_.compare_exchange_strong(expected, reason)) {
    const auto epoch = sleep_state_.load(std::memory_order_relaxed).epoch;
    LOG_TRACE() << "task with task_id="
                << GetTaskIdString(
                       current_task::GetCurrentTaskContextUnchecked())
                << " cancelled task with task_id=" << GetTaskIdString(this)
                << logging::LogExtra::Stacktrace();
    cancellation_reason_.store(reason, std::memory_order_relaxed);
    Wakeup(WakeupSource::kCancelRequest, epoch);
    task_processor_.GetTaskCounter().AccountTaskCancel();
  }
}

bool TaskContext::IsCancellable() const { return is_cancellable_; }

bool TaskContext::SetCancellable(bool value) {
  UASSERT(current_task::GetCurrentTaskContext() == this);
  UASSERT(state_ == Task::State::kRunning);

  return std::exchange(is_cancellable_, value);
}

// Should be defined before usage
auto TaskContext::UseWaitStrategy(WaitStrategy& wait_strategy) noexcept {
  class WaitManagerGuard {
   public:
    WaitManagerGuard(TaskContext& self, WaitStrategy& new_strategy) noexcept
        : self_(self), old_strategy_(self_.wait_manager_) {
      self_.wait_manager_ = &new_strategy;
    }

    WaitManagerGuard(const WaitManagerGuard&) = delete;
    WaitManagerGuard(WaitManagerGuard&&) = delete;

    ~WaitManagerGuard() { self_.wait_manager_ = old_strategy_; }

   private:
    TaskContext& self_;
    WaitStrategy* const old_strategy_;
  };

  return WaitManagerGuard{*this, wait_strategy};
}

TaskContext::WakeupSource TaskContext::Sleep(WaitStrategy& wait_strategy) {
  UASSERT(current_task::GetCurrentTaskContext() == this);
  UASSERT(state_ == Task::State::kRunning);

  /* ConditionVariable might call Sleep() inside of Sleep() due to
   * a lock in BeforeAwake, so store an old value on the stack.
   */
  const auto old_startegy_guard = UseWaitStrategy(wait_strategy);

  const auto sleep_epoch = sleep_state_.load().epoch;
  auto deadline = wait_manager_->GetDeadline();
  if (IsCancellable()) {
    if (cancel_deadline_ < deadline) deadline = cancel_deadline_;
  }
  ArmDeadlineTimer(deadline, sleep_epoch);

  yield_reason_ = YieldReason::kTaskWaiting;
  UASSERT(task_pipe_);
  TraceStateTransition(Task::State::kSuspended);
  ProfilerStopExecution();
  [[maybe_unused]] TaskContext* context = (*task_pipe_)().get();
  ProfilerStartExecution();
  TraceStateTransition(Task::State::kRunning);
  UASSERT(context == this);
  UASSERT(state_ == Task::State::kRunning);

  // Clear sleep_state_ now as we may sleep in BeforeAwake() below.
  const auto old_sleep_state = sleep_state_.exchange(
      MakeNextEpochSleepState(sleep_epoch), std::memory_order_acq_rel);

  // We compute the value here, because BeforeAwake() may lock some mutex. In
  // that case we'll be doing computations under a lock, we increase the
  // critical section and affect performance.
  const auto wakeup_source = GetPrimaryWakeupSource(old_sleep_state.flags);

  wait_strategy.BeforeAwake();
  wakeup_source_ = wakeup_source;
  return wakeup_source_;
}

void TaskContext::ArmDeadlineTimer(Deadline deadline,
                                   SleepState::Epoch sleep_epoch) {
  if (!deadline.IsReachable()) {
    return;
  }

  if (deadline.IsReached()) {
    Wakeup(WakeupSource::kDeadlineTimer, sleep_epoch);
    return;
  }

  boost::intrusive_ptr<TaskContext> ctx(this);
  auto callback = [ctx = std::move(ctx), sleep_epoch] {
    ctx->Wakeup(WakeupSource::kDeadlineTimer, sleep_epoch);
  };

  if (sleep_deadline_timer_) {
    sleep_deadline_timer_.Restart(std::move(callback), deadline);
  } else {
    sleep_deadline_timer_.Start(task_processor_.EventThreadPool().NextThread(),
                                std::move(callback), deadline);
  }
}

bool TaskContext::ShouldSchedule(SleepState::Flags prev_flags,
                                 WakeupSource source) {
  /* ShouldSchedule() returns true only for the first Wakeup().  All Wakeup()s
   * are serialized due to seq_cst in FetchOr().
   */

  if (!(prev_flags & SleepFlags::kSleeping)) return false;

  if (source == WakeupSource::kCancelRequest) {
    /* Don't wakeup if:
     * 1) kNonCancellable
     * 2) Other WakeupSource is already triggered
     */
    return prev_flags == SleepFlags::kSleeping;
  } else if (source == WakeupSource::kBootstrap) {
    return true;
  } else {
    if (prev_flags & SleepFlags::kNonCancellable) {
      /* If there was a cancellation request, but cancellation is blocked,
       * ignore it - we're the first to Schedule().
       */
      prev_flags.Clear(
          {SleepFlags::kNonCancellable, SleepFlags::kWakeupByCancelRequest});
    }

    /* Don't wakeup if:
     * 1) kNonCancellable and zero or more kCancelRequest triggered
     * 2) !kNonCancellable and any other WakeupSource is triggered
     */

    // We're the first to wakeup the baby
    return prev_flags == SleepFlags::kSleeping;
  }
}

void TaskContext::Wakeup(WakeupSource source, SleepState::Epoch epoch) {
  if (IsFinished()) return;

  auto prev_sleep_state = sleep_state_.load(std::memory_order_relaxed);
  do {
    if (prev_sleep_state.epoch != epoch) {
      // Epoch changed, wakeup is for some previous sleep
      return;
    }

    if (source == WakeupSource::kCancelRequest &&
        prev_sleep_state.flags & SleepFlags::kNonCancellable) {
      // We do not need to wakeup because:
      // - *this is non cancellable and the epoch is correct
      // - or even if the sleep_state_ changed and the task is now cancellable
      //   then epoch changed and wakeup request is not for the current sleep.
      return;
    }

    auto new_sleep_state = prev_sleep_state;
    new_sleep_state.flags |= static_cast<SleepFlags>(source);
    if (sleep_state_.compare_exchange_weak(prev_sleep_state, new_sleep_state,
                                           std::memory_order_relaxed,
                                           std::memory_order_relaxed)) {
      break;
    }
  } while (true);

  if (ShouldSchedule(prev_sleep_state.flags, source)) {
    Schedule();
  }
}

void TaskContext::Wakeup(WakeupSource source, TaskContext::NoEpoch) {
  UASSERT(source != WakeupSource::kDeadlineTimer);
  UASSERT(source != WakeupSource::kBootstrap);
  UASSERT(source != WakeupSource::kCancelRequest);

  if (IsFinished()) return;

  /* Set flag regardless of kSleeping - missing kSleeping usually means one of
   * the following: 1) the task is somewhere between Sleep() and setting
   * kSleeping in DoStep(). 2) the task is already awaken, but BeforeAwake() is
   * not yet finished (and not all timers/watchers are stopped).
   */
  const auto prev_sleep_state =
      FetchOr(sleep_state_, static_cast<SleepFlags>(source));
  if (ShouldSchedule(prev_sleep_state.flags, source)) {
    Schedule();
  }
}

TaskContext::WakeupSource TaskContext::DebugGetWakeupSource() const {
  UASSERT(current_task::GetCurrentTaskContext() == this);
  return wakeup_source_;
}

void TaskContext::CoroFunc(TaskPipe& task_pipe) {
  for (TaskContext* context : task_pipe) {
    UASSERT(context);
    context->yield_reason_ = YieldReason::kNone;
    context->task_pipe_ = &task_pipe;

    context->ProfilerStartExecution();

    // We only let tasks ran with CriticalAsync enter function body, others
    // get terminated ASAP.
    if (context->IsCancelRequested() && !context->WasStartedAsCritical()) {
      context->SetCancellable(false);
      // It is important to destroy payload here as someone may want
      // to synchronize in its dtor (e.g. lambda closure).
      {
        LocalStorageGuard local_storage_guard(*context);
        context->payload_ = {};
      }
      context->yield_reason_ = YieldReason::kTaskCancelled;
    } else {
      try {
        {
          // Destroy contents of LocalStorage in the coroutine
          // as dtors may want to schedule
          LocalStorageGuard local_storage_guard(*context);

          context->TraceStateTransition(Task::State::kRunning);
          CallOnce(context->payload_);
        }
        context->yield_reason_ = YieldReason::kTaskComplete;
      } catch (const CoroUnwinder&) {
        context->yield_reason_ = YieldReason::kTaskCancelled;
      }
    }

    context->ProfilerStopExecution();

    context->task_pipe_ = nullptr;
  }
}

void TaskContext::SetCancelDeadline(Deadline deadline) {
  UASSERT(current_task::GetCurrentTaskContext() == this);
  cancel_deadline_ = deadline;
}

bool TaskContext::HasLocalStorage() const { return local_storage_ != nullptr; }

LocalStorage& TaskContext::GetLocalStorage() { return *local_storage_; }

TaskContext::WakeupSource TaskContext::GetPrimaryWakeupSource(
    SleepState::Flags sleep_flags) {
  static constexpr std::pair<SleepState::Flags, WakeupSource> l[] = {
      {SleepFlags::kWakeupByWaitList, WakeupSource::kWaitList},
      {SleepFlags::kWakeupByDeadlineTimer, WakeupSource::kDeadlineTimer},
      {SleepFlags::kWakeupByBootstrap, WakeupSource::kBootstrap},
  };
  for (auto it : l)
    if (sleep_flags & it.first) return it.second;

  if ((sleep_flags & SleepFlags::kWakeupByCancelRequest) &&
      !(sleep_flags & SleepFlags::kNonCancellable))
    return WakeupSource::kCancelRequest;

  UASSERT_MSG(false, fmt::format("Cannot find valid wakeup source for {}",
                                 sleep_flags.GetValue()));
  throw std::logic_error(
      "Cannot find valid wakeup source, stacktrace:\n" +
      logging::stacktrace_cache::to_string(boost::stacktrace::stacktrace{}) +
      "\nvalue = " + std::to_string(sleep_flags.GetValue()));
}

bool TaskContext::WasStartedAsCritical() const { return is_critical_; }

void TaskContext::SetState(Task::State new_state) {
  auto old_state = Task::State::kNew;

  // CAS optimization
  switch (new_state) {
    case Task::State::kQueued:
      old_state = Task::State::kSuspended;
      break;
    case Task::State::kRunning:
      old_state = Task::State::kQueued;
      break;
    case Task::State::kSuspended:
    case Task::State::kCompleted:
      old_state = Task::State::kRunning;
      break;
    case Task::State::kCancelled:
      old_state = Task::State::kSuspended;
      break;
    case Task::State::kInvalid:
    case Task::State::kNew:
      UASSERT(!"Invalid new task state");
  }

  if (new_state == Task::State::kRunning ||
      new_state == Task::State::kSuspended) {
    if (new_state == Task::State::kRunning) {
      UASSERT(current_task::GetCurrentTaskContext() == this);
    } else {
      UASSERT(current_task::GetCurrentTaskContextUnchecked() == nullptr);
    }
    UASSERT(old_state == state_);
    // For kRunning we don't care other threads see old state_ (kQueued) for
    // some time.
    // For kSuspended synchronization point is DoStep()'s
    // sleep_state_.FetchOr().
    state_.store(new_state, std::memory_order_relaxed);
    return;
  }
  if (new_state == Task::State::kQueued) {
    UASSERT(old_state == state_ || state_ == Task::State::kNew);
    // Synchronization point is TaskProcessor::Schedule()
    state_.store(new_state, std::memory_order_relaxed);
    return;
  }

  // use strong CAS here to avoid losing transitions to finished
  while (!state_.compare_exchange_strong(old_state, new_state)) {
    if (old_state == new_state) {
      // someone else did the job
      return;
    }
    if (old_state == Task::State::kCompleted ||
        old_state == Task::State::kCancelled) {
      // leave as finished, do not wakeup
      return;
    }
  }

  if (IsFinished()) {
    sleep_deadline_timer_.Stop();

    finish_waiters_->WakeupOne();
  }
}

void TaskContext::Schedule() {
  UASSERT(state_ != Task::State::kQueued);
  SetState(Task::State::kQueued);
  TraceStateTransition(Task::State::kQueued);
  task_processor_.Schedule(this);
  // NOTE: may be executed at this point
}

void TaskContext::ProfilerStartExecution() {
  auto threshold_us = task_processor_.GetProfilerThreshold();
  if (threshold_us.count() > 0) {
    execute_started_ = std::chrono::steady_clock::now();
  } else {
    execute_started_ = {};
  }
}

void TaskContext::ProfilerStopExecution() {
  auto threshold_us = task_processor_.GetProfilerThreshold();
  if (threshold_us.count() <= 0) return;

  if (execute_started_ == std::chrono::steady_clock::time_point{}) {
    // the task was started w/o profiling, skip it
    return;
  }

  auto now = std::chrono::steady_clock::now();
  auto duration = now - execute_started_;
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(duration);

  task_processor_.GetTaskCounter().AccountTaskExecution(duration_us);

  if (duration_us >= threshold_us) {
    LOG_ERROR() << "Profiler threshold reached, task was executing "
                   "for too long without context switch ("
                << duration_us.count() << "us >= " << threshold_us.count()
                << "us)" << logging::LogExtra::Stacktrace();
  }
}

void TaskContext::TraceStateTransition(Task::State state) {
  if (trace_csw_left_ == 0) return;
  --trace_csw_left_;

  auto now = std::chrono::steady_clock::now();
  auto diff = now - last_state_change_timepoint_;
  if (last_state_change_timepoint_ == std::chrono::steady_clock::time_point())
    diff = {};
  auto diff_us =
      std::chrono::duration_cast<std::chrono::microseconds>(diff).count();
  last_state_change_timepoint_ = now;

  auto istate = Task::GetStateName(state);

  LOG_INFO_TO(task_processor_.GetTraceLogger())
      << "Task " << GetTaskId() << " changed state to " << istate
      << ", delay = " << diff_us << "us" << logging::LogExtra::Stacktrace();
}

bool TaskContext::CheckDeadline() {
  if (cancel_deadline_.IsReached()) {
    RequestCancel(TaskCancellationReason::kDeadline);
    return true;
  }
  return false;
}

}  // namespace impl
}  // namespace engine

#include "cxxabi_eh_globals.inc"
