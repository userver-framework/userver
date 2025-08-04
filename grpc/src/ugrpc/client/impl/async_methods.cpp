#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <fmt/format.h>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>
#include <userver/ugrpc/client/impl/tracing.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void SetStatusAndResetSpan(CallState& state, const grpc::Status& status) noexcept {
    SetStatusForSpan(state.GetSpan(), status);
    state.ResetSpan();
}

void SetErrorAndResetSpan(CallState& state, std::string_view error_message) noexcept {
    SetErrorForSpan(state.GetSpan(), error_message);
    state.ResetSpan();
}

}  // namespace

ugrpc::impl::AsyncMethodInvocation::WaitStatus WaitAndTryCancelIfNeeded(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    engine::Deadline deadline,
    grpc::ClientContext& context
) noexcept {
    const auto wait_status = invocation.WaitUntil(deadline);
    if (ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled == wait_status) {
        context.TryCancel();
    }
    return wait_status;
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus
WaitAndTryCancelIfNeeded(ugrpc::impl::AsyncMethodInvocation& invocation, grpc::ClientContext& context) noexcept {
    return WaitAndTryCancelIfNeeded(invocation, engine::Deadline{}, context);
}

void ProcessFinish(CallState& state, const google::protobuf::Message* final_response) {
    const auto& status = state.GetStatus();

    HandleCallStatistics(state, status);

    RunMiddlewarePipeline(state, FinishHooks(status, final_response));

    SetStatusAndResetSpan(state, status);
}

void ProcessFinishAbandoned(CallState& state) noexcept { SetStatusAndResetSpan(state, state.GetStatus()); }

void ProcessCancelled(CallState& state, std::string_view stage) noexcept {
    state.GetStatsScope().OnCancelled();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, fmt::format("Task cancellation at '{}'", stage));
}

void ProcessNetworkError(CallState& state, std::string_view stage) noexcept {
    state.GetStatsScope().OnNetworkError();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, fmt::format("Network error at '{}'", stage));
}

void CheckFinishStatus(CallState& state) {
    auto& status = state.GetStatus();
    if (!status.ok()) {
        ThrowErrorWithStatus(state.GetCallName(), std::move(status));
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
