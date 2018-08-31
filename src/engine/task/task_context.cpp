#include "task_context.hpp"

#include <cassert>
#include <exception>

#include <boost/core/ignore_unused.hpp>

#include <engine/coro/pool.hpp>
#include <engine/ev/timer.hpp>

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
  if (!current_task_context_ptr)
    throw std::logic_error(
        "current_task::GetCurrentTaskContext() called outside coroutine");
  return current_task_context_ptr;
}

impl::TaskContext* GetCurrentTaskContextUnchecked() {
  return current_task_context_ptr;
}

}  // namespace current_task

namespace impl {
namespace {

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
    func();
    func = {};
  }
}

}  // namespace

TaskContext::TaskContext(TaskProcessor& task_processor,
                         Task::Importance importance, Payload payload)
    : magic_(kMagic),
      task_processor_(task_processor),
      ref_(task_processor_),
      is_critical_(importance == Task::Importance::kCritical),
      payload_(std::move(payload)),
      state_(Task::State::kNew),
      is_detached_(false),
      is_cancellable_(true),
      cancellation_reason_(Task::CancellationReason::kNone),
      finish_waiters_(std::make_shared<WaitList>()),
      sleep_state_(SleepStateFlags::kSleeping),
      wakeup_source_(WakeupSource::kNone),
      task_pipe_(nullptr),
      yield_reason_(YieldReason::kNone) {
  assert(payload_);
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
    if (IsCancelRequested()) {
      SetState(Task::State::kCancelled);
      return;  // don't even start
    }

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
    case YieldReason::kTaskComplete:
      task_processor_.GetCoroPool().PutCoroutine(std::move(coro_));
      coro_.reset();
      SetState(Task::State::kCompleted);
      break;

    case YieldReason::kTaskCancelled:
      // unwound coroutine cannot be reused
      coro_.reset();
      SetState(Task::State::kCancelled);
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
    cancellation_reason_ = reason;
    Wakeup(WakeupSource::kCancelRequest);
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
  if (sleep_params_.deadline != Deadline{}) {
    auto time_left = sleep_params_.deadline - Deadline::clock::now();
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
  TaskContext* context = (*task_pipe_)().get();
  assert(context == this);
  boost::ignore_unused(context);

  if (deadline_timer) deadline_timer.Stop();

  if (wakeup_source_ != WakeupSource::kWaitList) {
    auto wait_list = sleep_params_.wait_list.lock();
    if (wait_list) {
      WaitList::Lock lock(*wait_list);
      wait_list->Remove(lock, this);
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

    try {
      // it is important to destroy payload here as someone may want
      // to synchronize in its dtor (e.g. lambda closure)
      CallOnce(context->payload_);
      context->yield_reason_ = YieldReason::kTaskComplete;
    } catch (const CoroUnwinder&) {
      context->yield_reason_ = YieldReason::kTaskCancelled;
    }

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

  if (!std::uncaught_exception() && SetCancellable(false)) {
    throw CoroUnwinder{};
  }
}

}  // namespace impl
}  // namespace engine
