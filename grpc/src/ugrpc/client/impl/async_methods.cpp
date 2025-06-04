#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <fmt/format.h>

#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>

#include <ugrpc/client/impl/tracing.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void ProcessCallStatistics(CallState& state, const grpc::Status& status) {
    auto& stats = state.GetStatsScope();
    stats.OnExplicitFinish(status.error_code());
    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED && state.IsDeadlinePropagated()) {
        stats.OnCancelledByDeadlinePropagation();
    }
    stats.Flush();
}

void SetStatusAndResetSpan(CallState& state, const grpc::Status& status) {
    SetStatusForSpan(state.GetSpan(), status);
    state.ResetSpan();
}

void SetErrorAndResetSpan(CallState& state, std::string_view error_message) {
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

void CheckOk(CallState& state, AsyncMethodInvocation::WaitStatus status, std::string_view stage) {
    if (status == impl::AsyncMethodInvocation::WaitStatus::kError) {
        state.SetFinished();
        state.GetStatsScope().OnNetworkError();
        state.GetStatsScope().Flush();
        SetErrorAndResetSpan(state, fmt::format("Network error at '{}'", stage));
        throw RpcInterruptedError(state.GetCallName(), stage);
    } else if (status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.SetFinished();
        state.GetStatsScope().OnCancelled();
        state.GetStatsScope().Flush();
        SetErrorAndResetSpan(state, fmt::format("Task cancellation at '{}'", stage));
        throw RpcCancelledError(state.GetCallName(), stage);
    }
}

void PrepareFinish(CallState& state) {
    UINVARIANT(!state.IsFinished(), "'Finish' called on a finished call");
    state.SetFinished();
}

void ProcessFinish(CallState& state, const google::protobuf::Message* final_response) {
    const auto& status = state.GetStatus();

    ProcessCallStatistics(state, status);

    if (final_response && status.ok()) {
        MiddlewarePipeline::PostRecvMessage(state, *final_response);
    }
    MiddlewarePipeline::PostFinish(state, status);

    SetStatusAndResetSpan(state, status);
}

void ProcessFinishCancelled(CallState& state) {
    state.GetStatsScope().OnCancelled();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, "Task cancellation at 'Finish'");
}

void CheckFinishStatus(CallState& state) {
    auto& status = state.GetStatus();
    if (!status.ok()) {
        ThrowErrorWithStatus(state.GetCallName(), std::move(status));
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
