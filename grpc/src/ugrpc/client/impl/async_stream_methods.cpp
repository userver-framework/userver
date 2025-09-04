#include <userver/ugrpc/client/impl/async_stream_methods.hpp>

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

ugrpc::impl::AsyncMethodInvocation::WaitStatus
WaitAndTryCancelIfNeeded(ugrpc::impl::AsyncMethodInvocation& invocation, grpc::ClientContext& context) noexcept {
    const auto wait_status = invocation.Wait();
    if (ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled == wait_status) {
        context.TryCancel();
    }
    return wait_status;
}

void CheckOk(StreamingCallState& state, ugrpc::impl::AsyncMethodInvocation::WaitStatus status, std::string_view stage) {
    if (status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError) {
        state.SetFinished();
        ProcessNetworkError(state, stage);
        throw RpcInterruptedError(state.GetCallName(), stage);
    } else if (status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
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

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
