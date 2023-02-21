#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

EventBase::~EventBase() = default;

void AsyncMethodInvocation::Notify(bool ok) noexcept {
  ok_ = ok;
  event_.Send();
}

void* EventBase::GetTag() noexcept { return static_cast<void*>(this); }

bool AsyncMethodInvocation::Wait() noexcept {
  event_.WaitNonCancellable();
  return ok_;
}

bool AsyncMethodInvocation::IsReady() const noexcept {
  return event_.IsReady();
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
