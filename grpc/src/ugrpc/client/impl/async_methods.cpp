#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <fmt/format.h>

#include <ugrpc/client/impl/tracing.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void ProcessCallStatistics(CallState& state) noexcept {
    const auto& status = state.GetStatus();
    auto& stats = state.GetStatsScope();
    stats.OnExplicitFinish(status.error_code());
    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED && state.IsDeadlinePropagated()) {
        stats.OnCancelledByDeadlinePropagation();
    }
    stats.Flush();
}

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
    ProcessCallStatistics(state);

    const auto& status = state.GetStatus();

    if (final_response && status.ok()) {
        MiddlewarePipeline::PostRecvMessage(state, *final_response);
    }

    MiddlewarePipeline::PostFinish(state);

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
