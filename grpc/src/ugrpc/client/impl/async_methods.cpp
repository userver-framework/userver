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
        // Cannot do stats_scope.Flush(), as it can be called in Notify concurrently.
        SetErrorAndResetSpan(state, fmt::format("Task cancellation at '{}'", stage));
        throw RpcCancelledError(state.GetCallName(), stage);
    }
}

void PrepareFinish(CallState& state) {
    UINVARIANT(!state.IsFinished(), "'Finish' called on a finished call");
    state.SetFinished();
}

void ProcessFinish(CallState& state, google::protobuf::Message* final_response) {
    const auto& status = state.GetStatus();

    state.GetStatsScope().OnExplicitFinish(status.error_code());
    state.GetStatsScope().Flush();

    if (final_response && status.ok()) {
        MiddlewarePipeline::PostRecvMessage(state, *final_response);
    }
    MiddlewarePipeline::PostFinish(state, status);

    SetStatusAndResetSpan(state, status);
}

void CheckFinishStatus(CallState& state) {
    auto& status = state.GetStatus();
    if (!status.ok()) {
        ThrowErrorWithStatus(state.GetCallName(), std::move(status));
    }
}

void ProcessFinishResult(
    CallState& state,
    AsyncMethodInvocation::WaitStatus wait_status,
    google::protobuf::Message* final_response,
    bool throw_on_error
) {
    if (wait_status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        state.GetStatsScope().OnCancelled();
        // Cannot do stats_scope.Flush(), as it can be called in Notify concurrently.
        state.GetContext().TryCancel();
        SetErrorAndResetSpan(state, "Task cancellation at 'Finish'");
        // Finish AsyncMethodInvocation will be awaited in its destructor.
        if (throw_on_error) {
            throw RpcCancelledError(state.GetCallName(), "Finish");
        }
        return;
    }

    UASSERT_MSG(
        wait_status == impl::AsyncMethodInvocation::WaitStatus::kOk,
        "ok=false in async Finish method invocation is prohibited "
        "by gRPC docs, see grpc::CompletionQueue::Next"
    );

    ProcessFinish(state, final_response);
    if (throw_on_error) {
        CheckFinishStatus(state);
    }
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
