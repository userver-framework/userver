#pragma once

#include <grpcpp/client_context.h>

#include <userver/ugrpc/impl/async_method_invocation.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

class RpcData;

/// AsyncMethodInvocation for Finish method that stops stats and Span timers
/// ASAP, without waiting for a Task to wake up
class FinishAsyncMethodInvocation final : public ugrpc::impl::AsyncMethodInvocation {
public:
    explicit FinishAsyncMethodInvocation(RpcData& data);

    ~FinishAsyncMethodInvocation() override;

    void Notify(bool ok) noexcept override;

private:
    RpcData& data_;
};

ugrpc::impl::AsyncMethodInvocation::WaitStatus
Wait(ugrpc::impl::AsyncMethodInvocation& invocation, grpc::ClientContext& context) noexcept;

ugrpc::impl::AsyncMethodInvocation::WaitStatus WaitUntil(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    grpc::ClientContext& context,
    engine::Deadline deadline
) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
