#include <userver/ugrpc/server/impl/async_method_invocation.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

RpcFinishedEvent::RpcFinishedEvent(
    engine::TaskCancellationToken cancellation_token,
    const grpc::ServerContext& server_ctx
) noexcept
    : cancellation_token_(std::move(cancellation_token)), server_ctx_(server_ctx) {}

void* RpcFinishedEvent::GetCompletionTag() noexcept { return static_cast<EventBase*>(this); }

void RpcFinishedEvent::WaitNonCancellable() noexcept { event_.WaitNonCancellable(); }

void RpcFinishedEvent::Notify(bool ok) noexcept {
    // From the documentation to grpcpp: Server-side AsyncNotifyWhenDone:
    // ok should always be true
    UASSERT(ok);
    if (server_ctx_.IsCancelled()) {
        cancellation_token_.RequestCancel();
    }
    event_.Send();
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
