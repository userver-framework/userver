#include <userver/ugrpc/client/impl/async_stream_methods.hpp>

#include <fmt/format.h>
#include <grpc/support/time.h>

#include <userver/utils/expected.hpp>

#include <userver/ugrpc/client/completion_status.hpp>
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

void HandleCallStatistics(CallState& state, const grpc::Status& status) noexcept {
    auto& stats = state.GetStatsScope();
    if (grpc::StatusCode::DEADLINE_EXCEEDED == status.error_code() && state.IsDeadlinePropagated()) {
        stats.OnCancelledByDeadlinePropagation();
    } else {
        stats.OnExplicitFinish(status.error_code());
    }
    stats.Flush();
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

void ProcessFinish(
    StreamingCallState& state,
    const CompletionStatus& completion_status,
    const google::protobuf::Message* response
) {
    UINVARIANT(completion_status.has_value(), "ProcessFinish must be called only with grpc::Status completions");
    const auto& status = completion_status.value();

    RunMiddlewarePipeline(state, MiddlewareHooks::FinishHooks(status, status.ok() ? response : nullptr));
    impl::HandleCallStatistics(state, status);
    SetStatusAndResetSpan(state, status);
}

void ProcessFinishAbandoned(StreamingCallState& state, const grpc::Status& status) noexcept {
    RunMiddlewarePipeline(
        state,
        MiddlewareHooks::FinishHooks(utils::unexpected{SpecialCaseCompletionType::kAbandoned}, nullptr)
    );

    // Nothing to do with statistics, `RpcStatisticsScope` automatically accounts "abandoned-error"
    SetStatusAndResetSpan(state, status);
}

void ProcessCancelled(StreamingCallState& state, std::string_view stage) noexcept {
    CompletionStatus result{utils::unexpected{SpecialCaseCompletionType::kCancelled}};
    RunMiddlewarePipeline(state, MiddlewareHooks::FinishHooks(result, nullptr));

    state.GetStatsScope().OnCancelled();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, fmt::format("Task cancellation at '{}'", stage));
}

void ProcessNetworkError(StreamingCallState& state, std::string_view stage) noexcept {
    CompletionStatus result{utils::unexpected{SpecialCaseCompletionType::kNetworkError}};
    RunMiddlewarePipeline(state, MiddlewareHooks::FinishHooks(result, nullptr));

    state.GetStatsScope().OnNetworkError();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, fmt::format("Network error at '{}'", stage));
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
