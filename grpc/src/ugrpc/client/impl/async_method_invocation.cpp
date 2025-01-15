#include <userver/ugrpc/client/impl/async_method_invocation.hpp>

#include <userver/ugrpc/client/impl/async_methods.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

FinishAsyncMethodInvocation::FinishAsyncMethodInvocation(RpcData& data) : data_(data) {}

FinishAsyncMethodInvocation::~FinishAsyncMethodInvocation() { WaitWhileBusy(); }

void FinishAsyncMethodInvocation::Notify(bool ok) noexcept {
    if (ok) {
        try {
            auto& status = data_.GetStatus();
            data_.GetStatsScope().OnExplicitFinish(status.error_code());

            if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED && data_.IsDeadlinePropagated()) {
                data_.GetStatsScope().OnCancelledByDeadlinePropagation();
            }

            data_.GetStatsScope().Flush();
        } catch (const std::exception& ex) {
            LOG_LIMITED_ERROR() << "Error in FinishAsyncMethodInvocation::Notify: " << ex;
        }
    }
    AsyncMethodInvocation::Notify(ok);
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus
Wait(ugrpc::impl::AsyncMethodInvocation& invocation, grpc::ClientContext& context) noexcept {
    return impl::WaitUntil(invocation, context, engine::Deadline{});
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus WaitUntil(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    grpc::ClientContext& context,
    engine::Deadline deadline
) noexcept {
    const auto status = invocation.WaitUntil(deadline);
    if (status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        context.TryCancel();
    }

    return status;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
