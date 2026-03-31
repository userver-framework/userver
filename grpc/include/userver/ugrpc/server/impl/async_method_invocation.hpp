#pragma once

#include <grpcpp/server_context.h>

#include <userver/engine/single_use_event.hpp>
#include <userver/engine/task/cancel.hpp>

#include <userver/ugrpc/impl/event_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

class RpcDoneEvent final : public ugrpc::impl::EventBase {
public:
    RpcDoneEvent(engine::TaskCancellationToken cancellation_token, const grpc::ServerContext& server_context) noexcept;

    RpcDoneEvent(const RpcDoneEvent&) = delete;
    RpcDoneEvent& operator=(const RpcDoneEvent&) = delete;
    RpcDoneEvent(RpcDoneEvent&&) = delete;
    RpcDoneEvent& operator=(RpcDoneEvent&&) = delete;

    void* GetCompletionTag() noexcept;

    /// @see EventBase::Notify
    void Notify(bool ok) noexcept override;

    /// @brief For use from coroutines
    void WaitNonCancellable() noexcept;

private:
    engine::TaskCancellationToken cancellation_token_;
    const grpc::ServerContext& server_context_;
    engine::SingleUseEvent event_;
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
