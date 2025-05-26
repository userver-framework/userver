#include <userver/ugrpc/client/impl/async_method_invocation.hpp>

#include <userver/ugrpc/client/impl/call_state.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

FinishAsyncMethodInvocation::FinishAsyncMethodInvocation(CallState& state) : state_(state) {}

FinishAsyncMethodInvocation::~FinishAsyncMethodInvocation() { WaitWhileBusy(); }

void FinishAsyncMethodInvocation::Notify(bool ok) noexcept {
    if (ok) {
        try {
            auto& status = state_.GetStatus();
            state_.GetStatsScope().OnExplicitFinish(status.error_code());

            if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED && state_.IsDeadlinePropagated()) {
                state_.GetStatsScope().OnCancelledByDeadlinePropagation();
            }

            state_.GetStatsScope().Flush();
        } catch (const std::exception& ex) {
            LOG_LIMITED_ERROR() << "Error in FinishAsyncMethodInvocation::Notify: " << ex;
        } catch (...) {
            // We have to catch any exception to notify a task in any case.
            LOG_LIMITED_ERROR() << "Error in FinishAsyncMethodInvocation::Notify: <non std::exception-based>";
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
