#include <userver/ugrpc/impl/async_method_invocation.hpp>

#include <userver/engine/task/cancel.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

EventBase::~EventBase() = default;

AsyncMethodInvocation::~AsyncMethodInvocation() { WaitWhileBusy(); }

void AsyncMethodInvocation::Notify(bool ok) noexcept {
  ok_ = ok;
  event_.Send();
}

bool AsyncMethodInvocation::IsBusy() const noexcept { return busy_; }

void* AsyncMethodInvocation::GetTag() noexcept {
  UASSERT(!busy_);
  busy_ = true;
  return static_cast<EventBase*>(this);
}

AsyncMethodInvocation::WaitStatus AsyncMethodInvocation::Wait() noexcept {
  return WaitUntil(engine::Deadline{});
}

AsyncMethodInvocation::WaitStatus AsyncMethodInvocation::WaitUntil(
    engine::Deadline deadline) noexcept {
  if (engine::current_task::ShouldCancel()) {
    // Make sure that cancelled RPC returns kCancelled (significant for tests)
    return WaitStatus::kCancelled;
  }

  if (!event_.WaitForEventUntil(deadline)) {
    if (engine::current_task::ShouldCancel()) {
      return WaitStatus::kCancelled;
    } else {
      return WaitStatus::kDeadline;
    }
  }

  busy_ = false;
  return ok_ ? WaitStatus::kOk : WaitStatus::kError;
}

bool AsyncMethodInvocation::IsReady() const noexcept {
  return event_.IsReady();
}

void AsyncMethodInvocation::WaitWhileBusy() {
  if (busy_) {
    engine::TaskCancellationBlocker blocker;
    const auto result = event_.WaitForEvent();
    UASSERT(result);
  }
  busy_ = false;
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
