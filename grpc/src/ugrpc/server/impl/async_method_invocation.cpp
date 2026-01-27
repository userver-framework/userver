#include <userver/ugrpc/server/impl/async_method_invocation.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

RpcDoneEvent::RpcDoneEvent(
    engine::TaskCancellationToken cancellation_token,
    const grpc::ServerContext& server_context
) noexcept
    : cancellation_token_(std::move(cancellation_token)), server_context_(server_context) {}

void* RpcDoneEvent::GetCompletionTag() noexcept { return static_cast<EventBase*>(this); }

void RpcDoneEvent::WaitNonCancellable() noexcept { event_.WaitNonCancellable(); }

void RpcDoneEvent::Notify(bool ok) noexcept {
    // From the documentation to grpcpp: Server-side AsyncNotifyWhenDone:
    // ok should always be true
    UASSERT(ok);
    if (server_context_.IsCancelled()) {
        cancellation_token_.RequestCancel();
    }
    event_.Send();
}

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
