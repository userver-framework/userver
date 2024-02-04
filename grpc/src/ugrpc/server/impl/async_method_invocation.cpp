#include <userver/ugrpc/server/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

RpcFinishedEvent::RpcFinishedEvent(
    engine::TaskCancellationToken cancellation_token,
    grpc::ServerContext& server_ctx) noexcept
    : cancellation_token_(std::move(cancellation_token)),
      server_ctx_(server_ctx) {}

void* RpcFinishedEvent::GetTag() noexcept { return this; }

void RpcFinishedEvent::Wait() noexcept { event_.WaitNonCancellable(); }

void RpcFinishedEvent::Notify(bool ok) noexcept {
  // From the documentation to grpcpp: Server-side AsyncNotifyWhenDone:
  // ok should always be true
  UASSERT(ok);
  if (server_ctx_.IsCancelled()) {
    cancellation_token_.RequestCancel();
  }
  event_.Send();
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus Wait(
    ugrpc::impl::AsyncMethodInvocation& async) {
  using WaitStatus = ugrpc::impl::AsyncMethodInvocation::WaitStatus;

  const engine::TaskCancellationBlocker blocker;
  const auto status = async.Wait();

  switch (status) {
    case WaitStatus::kOk:
    case WaitStatus::kError:
      return status;
    case WaitStatus::kCancelled:
    case WaitStatus::kDeadline:
      UASSERT(false);
  }

  UASSERT(false);
  return status;
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
