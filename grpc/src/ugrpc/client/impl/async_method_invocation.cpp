#include <userver/ugrpc/client/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

ugrpc::impl::AsyncMethodInvocation::WaitStatus Wait(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    grpc::ClientContext& context) noexcept {
  const auto status = invocation.Wait();
  if (status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled)
    context.TryCancel();

  return status;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
