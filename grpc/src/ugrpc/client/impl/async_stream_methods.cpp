#include <userver/ugrpc/client/impl/async_stream_methods.hpp>

#include <fmt/format.h>
#include <grpc/support/time.h>

#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>
#include <userver/ugrpc/client/impl/tracing.hpp>
#include <userver/ugrpc/time_utils.hpp>

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

void ThrowIfDeadlineIsExceeded(grpc::ClientContext& client_context, std::string_view call_name) {
    const auto raw_deadline = client_context.raw_deadline();
    const auto deadline = ugrpc::TimespecToDeadline(raw_deadline);
    if (deadline.IsReached()) {
        grpc::Status deadline_status(grpc::StatusCode::DEADLINE_EXCEEDED, "Deadline exceeded");
        ThrowErrorWithStatus(call_name, std::move(deadline_status));
    }
}

ugrpc::impl::AsyncMethodInvocation::WaitStatus WaitAndTryCancelIfNeeded(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    grpc::ClientContext& client_context
) noexcept {
    const auto wait_status = invocation.Wait();
    if (ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled == wait_status) {
        client_context.TryCancel();
    }
    return wait_status;
}

void CheckOk(
    StreamingCallState& state,
    ugrpc::impl::AsyncMethodInvocation::WaitStatus wait_status,
    std::string_view stage
) {
    if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError) {
        state.SetFinished();
        ThrowIfDeadlineIsExceeded(state.GetClientContext(), state.GetCallName());
        ProcessNetworkError(state, stage);
        throw RpcInterruptedError(state.GetCallName(), stage);
    } else if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.SetFinished();
        ProcessCancelled(state, stage);
        throw RpcCancelledError(state.GetCallName(), stage);
    }
}

void CheckFinishStatus(CallState& state) {
    auto& status = state.GetStatus();
    if (!status.ok()) {
        ThrowErrorWithStatus(state.GetCallName(), std::move(status));
    }
}

void ProcessFinish(CallState& state, const grpc::Status& status, const google::protobuf::Message* response) {
    RunMiddlewarePipeline(state, MiddlewareHooks::FinishHooks(status, status.ok() ? response : nullptr));

    HandleCallStatistics(state, status);

    SetStatusAndResetSpan(state, status);
}

void ProcessFinishAbandoned(CallState& state, const grpc::Status& status) noexcept {
    // Nothing to do with statistics, `RpcStatisticsScope` automatically accounts "abandoned-error"
    SetStatusAndResetSpan(state, status);
}

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

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
