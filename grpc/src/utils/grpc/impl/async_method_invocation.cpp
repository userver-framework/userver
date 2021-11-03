#include <userver/utils/grpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::grpc::impl {

void AsyncMethodInvocation::Notify(bool ok) noexcept {
  ok_ = ok;
  event_.Send();
}

void* AsyncMethodInvocation::GetTag() noexcept {
  return static_cast<void*>(this);
}

bool AsyncMethodInvocation::Wait() noexcept {
  event_.WaitNonCancellable();
  return ok_;
}

}  // namespace utils::grpc::impl

USERVER_NAMESPACE_END
