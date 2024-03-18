#include <userver/engine/single_use_event.hpp>

#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>

#include <engine/impl/future_utils.hpp>
#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

// Not considered implicitly noexcept on gcc-9.
// NOLINTNEXTLINE(hicpp-use-equals-default,modernize-use-equals-default)
SingleUseEvent::SingleUseEvent() noexcept {}

SingleUseEvent::~SingleUseEvent() = default;

void SingleUseEvent::Wait() {
  switch (WaitUntil(Deadline{})) {
    case FutureStatus::kReady:
      break;
    case FutureStatus::kTimeout:
      UASSERT_MSG(false,
                  "Timeout is not expected here due to unreachable "
                  "Deadline at Sleep");
#ifdef NDEBUG
      [[fallthrough]];
#endif
    case FutureStatus::kCancelled:
      throw WaitInterruptedException(current_task::CancellationReason());
  }
}

FutureStatus SingleUseEvent::WaitUntil(Deadline deadline) {
  impl::TaskContext& current = current_task::GetCurrentTaskContext();
  impl::FutureWaitStrategy wait_strategy{*this, current};
  const auto wakeup_source = current.Sleep(wait_strategy, deadline);

  // There are no spurious wakeups, because the event is single-use: if a task
  // has ever been notified by this SingleUseEvent, then the task will find
  // the SingleUseEvent ready once it wakes up.
  if (wakeup_source == impl::TaskContext::WakeupSource::kWaitList) {
    UASSERT(waiters_->IsSignaled());
  }
  return impl::ToFutureStatus(wakeup_source);
}

void SingleUseEvent::WaitNonCancellable() noexcept {
  const TaskCancellationBlocker cancellation_blocker;

  switch (WaitUntil(Deadline{})) {
    case FutureStatus::kReady:
      break;
    case FutureStatus::kTimeout:
      UASSERT_MSG(false,
                  "Timeout is not expected here due to unreachable "
                  "Deadline at Sleep");
      break;
    case FutureStatus::kCancelled:
      UASSERT_MSG(false,
                  "Cancellation should have been blocked "
                  "by TaskCancellationBlocker");
      break;
  }
}

void SingleUseEvent::Send() noexcept {
  UASSERT_MSG(!waiters_->IsSignaled(),
              "Multiple producers detected for the same SingleUseEvent");
  waiters_->SetSignalAndWakeupOne();
}

bool SingleUseEvent::IsReady() const noexcept { return waiters_->IsSignaled(); }

impl::EarlyWakeup SingleUseEvent::TryAppendWaiter(impl::TaskContext& waiter) {
  return impl::EarlyWakeup{waiters_->GetSignalOrAppend(&waiter)};
}

void SingleUseEvent::RemoveWaiter(impl::TaskContext& waiter) noexcept {
  waiters_->Remove(waiter);
}

void SingleUseEvent::RethrowErrorResult() const {
  // TODO support failure states in SingleUseEvent, for WaitAllChecked?
}

void SingleUseEvent::AfterWait() noexcept {}

}  // namespace engine

USERVER_NAMESPACE_END
