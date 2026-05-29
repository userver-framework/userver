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

void TryRunMiddlewarePipeline(CallState& state, bool throw_on_error, const MiddlewareHooks& hooks) {
    try {
        RunMiddlewarePipeline(state, hooks);
    } catch (std::exception& ex) {
        LOG_WARNING() << "There is a caught exception in 'Finish': " << ex;
        if (throw_on_error) {
            throw;
        }
    }
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
        ProcessNetworkError(state, true, stage);
    } else if (wait_status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.SetFinished();
        if (impl::IsTaskCancelledByDeadlinePropagation()) {
            ProcessTimeoutDeadlinePropagated(state, true, stage);
        } else {
            ProcessCancelled(state, true, stage);
        }
    }
}

void ProcessFinish(
    StreamingCallState& state,
    bool throw_on_error,
    CompletionStatus&& completion_status,
    const google::protobuf::Message* response
) {
    UINVARIANT(completion_status.has_value(), "ProcessFinish must be called only with grpc::Status completions");
    auto& status = completion_status.value();

    TryRunMiddlewarePipeline(
        state,
        throw_on_error,
        MiddlewareHooks::FinishHooks(status, status.ok() ? response : nullptr)
    );

    state.GetStatsScope().OnExplicitFinish(status.error_code());
    state.GetStatsScope().Flush();
    SetStatusAndResetSpan(state, status);

    if (throw_on_error && !status.ok()) {
        ThrowErrorWithStatus(state.GetCallName(), std::move(status));
    }
}

void ProcessTimeoutDeadlinePropagated(StreamingCallState& state, bool throw_on_error, std::string_view stage) {
    TryRunMiddlewarePipeline(
        state,
        throw_on_error,
        MiddlewareHooks::FinishHooks(utils::unexpected{SpecialCaseCompletionType::kTimeoutDeadlinePropagated}, nullptr)
    );

    state.GetStatsScope().OnCancelledByDeadlinePropagation();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, fmt::format("Deadline propagated at '{}'", stage));

    if (throw_on_error) {
        throw RpcCancelledError(state.GetCallName(), fmt::format("{} (Deadline Propagation)", stage));
    }
}

void ProcessCancelled(StreamingCallState& state, bool throw_on_error, std::string_view stage) {
    CompletionStatus result{utils::unexpected{SpecialCaseCompletionType::kCancelled}};
    TryRunMiddlewarePipeline(state, throw_on_error, MiddlewareHooks::FinishHooks(result, nullptr));

    state.GetStatsScope().OnCancelled();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, fmt::format("Task cancellation at '{}'", stage));

    if (throw_on_error) {
        throw RpcCancelledError(state.GetCallName(), stage);
    }
}

void ProcessNetworkError(StreamingCallState& state, bool throw_on_error, std::string_view stage) {
    CompletionStatus result{utils::unexpected{SpecialCaseCompletionType::kNetworkError}};
    TryRunMiddlewarePipeline(state, throw_on_error, MiddlewareHooks::FinishHooks(result, nullptr));

    state.GetStatsScope().OnNetworkError();
    state.GetStatsScope().Flush();
    SetErrorAndResetSpan(state, fmt::format("Network error at '{}'", stage));

    if (throw_on_error) {
        throw RpcInterruptedError(state.GetCallName(), stage);
    }
}

void ProcessFinishAbandoned(StreamingCallState& state, const grpc::Status& status) noexcept {
    TryRunMiddlewarePipeline(
        state,
        false,
        MiddlewareHooks::FinishHooks(utils::unexpected{SpecialCaseCompletionType::kAbandoned}, nullptr)
    );

    // Nothing to do with statistics, `RpcStatisticsScope` automatically accounts "abandoned-error"
    SetStatusAndResetSpan(state, status);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
