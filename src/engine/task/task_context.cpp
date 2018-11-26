#include "task_context.hpp"

#include <cassert>
#include <cctype>
#include <exception>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/core/demangle.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/stacktrace.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/timer.hpp>
#include <logging/log_extra.hpp>

#include "task_processor.hpp"

namespace engine {
namespace current_task {
namespace {

thread_local impl::TaskContext* current_task_context_ptr = nullptr;

void SetCurrentTaskContext(impl::TaskContext* context) {
  assert(!current_task_context_ptr || !context);
  current_task_context_ptr = context;
}

}  // namespace

impl::TaskContext* GetCurrentTaskContext() {
  assert(current_task_context_ptr);
  if (!current_task_context_ptr) {
    LOG_ERROR()
        << "current_task::GetCurrentTaskContext() called outside coroutine"
        << logging::LogExtra::Stacktrace();
    throw std::logic_error(
        "current_task::GetCurrentTaskContext() called outside coroutine. "
        "stacktrace:\n" +
        to_string(boost::stacktrace::stacktrace{}));
  }
  return current_task_context_ptr;
}

impl::TaskContext* GetCurrentTaskContextUnchecked() {
  return current_task_context_ptr;
}

}  // namespace current_task

namespace impl {
namespace {

std::string GetTaskIdString(const impl::TaskContext* task) {
  return std::to_string(task ? task->GetTaskId() : 0);
}

// we don't use native boost.coroutine stack unwinding mechanisms for cancel
// as they don't allow us to yield from unwind
// e.g. to serialize nested tasks destruction
//
// XXX: Standard library has catch-all in some places, namely streams.
// With libstdc++ we can use magic __forced_unwind exception class
// but libc++ has nothing of the like.
// It's possile that we will have to use low-level C++ ABI for forced unwinds
// here, but for now this'll suffice.
class CoroUnwinder {};

class CurrentTaskScope {
 public:
  explicit CurrentTaskScope(TaskContext* context) {
    current_task::SetCurrentTaskContext(context);
  }
  ~CurrentTaskScope() { current_task::SetCurrentTaskContext(nullptr); }
};

template <typename Func>
void CallOnce(Func& func) {
  if (func) {
    auto func_to_destroy = std::move(func);
    func_to_destroy();
  }
}

// Heuristic to detect calls from destructors.
// It is not perfect as it is easily broken by destructor inlining, but it may
// help in some cases.
bool IsExecutingDestructor() {
  static const std::string kMangledNamePrefix = "_Z";

  for (const auto& frame : boost::stacktrace::stacktrace{}) {
    auto func_name = frame.name();

    if (boost::algorithm::starts_with(func_name, kMangledNamePrefix)) {
      // XXX: Demangler in xenial doesn't know about decltype(auto),
      // change it to auto. We may change some names, but they're not relevant.
      boost::algorithm::replace_all(func_name, "Dc", "Da");
      func_name = boost::core::demangle(func_name.c_str());
    }

    if (!boost::algorithm::starts_with(func_name, kMangledNamePrefix)) {
      // Look for dtors (~Name)
      for (auto pos = func_name.find('~');
           pos != func_name.npos && pos + 1 < func_name.size();
           pos = func_name.find('~', pos + 1)) {
        // operator~()
        if (func_name[pos + 1] == '(') continue;
        // nested members (lambdas, classes etc.)
        if (func_name.find(':', pos + 1) != func_name.npos) continue;

        return true;
      }
    } else {
      // For some mysterious reason name wasn't demangled, using safe mode.
      // Look for "D*" pattern, where * is digit (actually 0, 1 or 2)
      for (auto pos = func_name.find('D');
           pos != func_name.npos && pos + 1 < func_name.size();
           pos = func_name.find('D', pos + 1)) {
        // definitely not mangled dtor token
        if (!std::isdigit(func_name[pos + 1])) continue;

        return true;
      }
    }
  }
  return false;
}

}  // namespace

TaskContext::TaskContext(TaskProcessor& task_processor,
                         Task::Importance importance, Payload payload)
    : magic_(kMagic),
      task_processor_(task_processor),
      task_counter_token_(task_processor_.GetTaskCounter()),
      is_critical_(importance == Task::Importance::kCritical),
      payload_(std::move(payload)),
      state_(Task::State::kNew),
      is_detached_(false),
      is_cancellable_(true),
      cancellation_reason_(Task::CancellationReason::kNone),
      finish_waiters_(std::make_shared<WaitList>()),
      task_queue_wait_timepoint_(),
      sleep_state_(SleepStateFlags::kSleeping),
      wakeup_source_(WakeupSource::kNone),
      task_pipe_(nullptr),
      yield_reason_(YieldReason::kNone) {
  assert(payload_);
  LOG_TRACE() << "task with task_id="
              << GetTaskIdString(current_task::GetCurrentTaskContextUnchecked())
              << " created task with task_id=" << GetTaskIdString(this)
              << logging::LogExtra::Stacktrace();
}

TaskContext::~TaskContext() {
  LOG_TRACE() << "Task with task_id=" << GetTaskIdString(this) << " stopped"
              << logging::LogExtra::Stacktrace();
}

bool TaskContext::IsCritical() const {
  // running tasks must not be susceptible to overload
  // e.g. we might need to run coroutine to cancel it
  return is_critical_ || coro_;
}

void TaskContext::SetDetached() {
  bool was_detached = is_detached_.exchange(true);
  assert(!was_detached);
  boost::ignore_unused(was_detached);
}

void TaskContext::Wait() const { WaitUntil({}); }

void TaskContext::WaitUntil(Deadline deadline) const {
  if (IsFinished()) return;

  WaitList::Lock lock(*finish_waiters_);
  auto caller_ctx = current_task::GetCurrentTaskContext();
  SleepParams new_sleep_params;
  new_sleep_params.deadline = std::move(deadline);
  new_sleep_params.wait_list = finish_waiters_;
  new_sleep_params.exec_after_asleep = [this, &lock, caller_ctx] {
    finish_waiters_->Append(lock, caller_ctx);
    lock.Release();
  };
  if (IsFinished()) return;  // try to avoid ctx switch if possible
  caller_ctx->Sleep(std::move(new_sleep_params));
}

void TaskContext::DoStep() {
  if (IsFinished()) return;

  if (!coro_) {
    coro_ = task_processor_.GetCoroPool().GetCoroutine();
  }

  sleep_state_ = SleepStateFlags::kNone;
  SetState(Task::State::kRunning);
  {
    CurrentTaskScope current_task(this);
    (*coro_)(this);
    CallOnce(sleep_params_.exec_after_asleep);
  }

  switch (yield_reason_) {
    case YieldReason::kTaskCancelled:
    case YieldReason::kTaskComplete:
      task_processor_.GetCoroPool().PutCoroutine(std::move(coro_));
      coro_.reset();
      SetState(yield_reason_ == YieldReason::kTaskComplete
                   ? Task::State::kCompleted
                   : Task::State::kCancelled);
      break;

    case YieldReason::kTaskWaiting:
      SetState(Task::State::kSuspended);
      {
        auto prev_sleep_state =
            sleep_state_.FetchOr(SleepStateFlags::kSleeping);
        if ((prev_sleep_state & SleepStateFlags::kWakeupRequested) &&
            !(prev_sleep_state & SleepStateFlags::kSleeping)) {
          Schedule();
        }
      }
      break;

    case YieldReason::kNone:
      assert(!"invalid yield reason");
      throw std::logic_error("invalid yield reason");
  }
}

void TaskContext::RequestCancel(Task::CancellationReason reason) {
  auto expected = Task::CancellationReason::kNone;
  if (cancellation_reason_.compare_exchange_strong(expected, reason)) {
    LOG_TRACE() << "task with task_id="
                << GetTaskIdString(
                       current_task::GetCurrentTaskContextUnchecked())
                << " cancelled task with task_id=" << GetTaskIdString(this)
                << logging::LogExtra::Stacktrace();
    cancellation_reason_ = reason;
    Wakeup(WakeupSource::kCancelRequest);
    task_processor_.GetTaskCounter().AccountTaskCancel();
  }
}

bool TaskContext::SetCancellable(bool value) {
  return is_cancellable_.exchange(value);
}

void TaskContext::Sleep(SleepParams&& sleep_params) {
  assert(current_task::GetCurrentTaskContext() == this);
  assert(state_ == Task::State::kRunning);

  // don't sleep if we're going to cancel anyway
  if (IsCancelRequested()) Unwind();

  sleep_params_ = std::move(sleep_params);
  ev::Timer deadline_timer;
  if (sleep_params_.deadline.IsReachable()) {
    const auto time_left = sleep_params_.deadline.TimeLeft();
    if (time_left.count() > 0) {
      boost::intrusive_ptr<TaskContext> ctx(this);
      deadline_timer = ev::Timer(
          task_processor_.EventThreadPool().NextThread(),
          [ctx = std::move(ctx)] { ctx->Wakeup(WakeupSource::kDeadlineTimer); },
          time_left);
    } else {
      Wakeup(WakeupSource::kDeadlineTimer);
    }
  }

  yield_reason_ = YieldReason::kTaskWaiting;
  assert(task_pipe_);
  ProfilerStopExecution();
  TaskContext* context = (*task_pipe_)().get();
  ProfilerStartExecution();
  assert(context == this);
  boost::ignore_unused(context);

  if (deadline_timer) deadline_timer.Stop();

  if (wakeup_source_ != WakeupSource::kWaitList) {
    auto wait_list = sleep_params_.wait_list.lock();
    if (wait_list) {
      wait_list->Remove(this);
    }
  }
  CallOnce(sleep_params_.exec_before_awake);

  sleep_params_ = {};
  // reset state again in case timer has fired during wakeup
  sleep_state_ = SleepStateFlags::kNone;

  if (IsCancelRequested()) Unwind();
}

void TaskContext::Wakeup(WakeupSource source) {
  if (IsFinished()) return;
  auto prev_sleep_state =
      sleep_state_.FetchOr(SleepStateFlags::kWakeupRequested);
  if (!(prev_sleep_state & SleepStateFlags::kWakeupRequested)) {
    wakeup_source_ = source;
    if (prev_sleep_state & SleepStateFlags::kSleeping) {
      Schedule();
    }
  }
}

void TaskContext::CoroFunc(TaskPipe& task_pipe) {
  for (TaskContext* context : task_pipe) {
    assert(context);
    context->yield_reason_ = YieldReason::kNone;
    context->task_pipe_ = &task_pipe;

    context->ProfilerStartExecution();

    // it is important to destroy payload here as someone may want
    // to synchronize in its dtor (e.g. lambda closure)
    if (context->IsCancelRequested()) {
      context->SetCancellable(false);
      context->payload_ = {};
      context->yield_reason_ = YieldReason::kTaskCancelled;
    } else {
      try {
        CallOnce(context->payload_);
        context->yield_reason_ = YieldReason::kTaskComplete;
      } catch (const CoroUnwinder&) {
        context->yield_reason_ = YieldReason::kTaskCancelled;
      }
    }

    context->ProfilerStopExecution();

    context->task_pipe_ = nullptr;
  }
}

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
      assert(!"Invalid new task state");
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
    WaitList::Lock lock(*finish_waiters_);
    finish_waiters_->WakeupAll(lock);
  }
}

void TaskContext::Schedule() {
  assert(state_ != Task::State::kQueued);
  SetState(Task::State::kQueued);
  task_processor_.Schedule(this);
  // NOTE: may be executed at this point
}

void TaskContext::Unwind() {
  assert(current_task::GetCurrentTaskContext() == this);
  assert(state_ == Task::State::kRunning);

  if (std::uncaught_exception()) return;
  if (IsExecutingDestructor()) {
    LOG_WARNING() << "Attempt to cancel a task while executing destructor"
                  << logging::LogExtra::Stacktrace();
    return;
  }
  if (SetCancellable(false)) {
    LOG_TRACE() << "Cancelling current task" << logging::LogExtra::Stacktrace();
    throw CoroUnwinder{};
  }
}

#ifdef USERVER_PROFILER
void TaskContext::ProfilerStartExecution() {
  execute_started_ = std::chrono::steady_clock::now();
}

void TaskContext::ProfilerStopExecution() {
  auto now = std::chrono::steady_clock::now();
  auto duration = now - execute_started_;
  auto duration_us =
      std::chrono::duration_cast<std::chrono::microseconds>(duration);
  auto threshold_us = task_processor_.GetProfilerThreshold();

  task_processor_.GetTaskCounter().AccountTaskExecution(duration_us);

  if (duration_us >= threshold_us) {
    LOG_ERROR() << "Profiler threshold reached, task was executing "
                   "for too long without context switch ("
                << duration_us.count() << "us >= " << threshold_us.count()
                << "us)" << logging::LogExtra::Stacktrace();
  }
}
#else  // USERVER_PROFILER
void TaskContext::ProfilerStartExecution() {}

void TaskContext::ProfilerStopExecution() {}
#endif

}  // namespace impl
}  // namespace engine

#include "cxxabi_eh_globals.inc"
