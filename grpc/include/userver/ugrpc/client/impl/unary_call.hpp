#pragma once

#include <fmt/format.h>
#include <google/protobuf/message.h>
#include <grpcpp/support/async_unary_call.h>

#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/call_context.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/ugrpc/client/impl/call_params.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/client/impl/middleware_pipeline.hpp>
#include <userver/ugrpc/client/impl/prepare_call.hpp>
#include <userver/ugrpc/client/impl/retry_backoff.hpp>
#include <userver/ugrpc/client/impl/retry_policy.hpp>
#include <userver/ugrpc/client/impl/tracing.hpp>
#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

template <typename Stub, typename Request, typename Response>
class UnaryCall final {
public:
    using PrepareUnaryCall = PrepareUnaryCallProxy<Stub, Request, Response>;

    UnaryCall(CallParams&& params, PrepareUnaryCall&& prepare_unary_call, const Request& request)
        : call_options_{std::move(params.call_options)},
          state_{std::move(params), CallKind::kUnaryCall, /*setup_client_context*/ false},
          context_{utils::impl::InternalTag{}, state_},
          prepare_unary_call_{std::move(prepare_unary_call)},
          request_{request} {}

    ~UnaryCall() = default;

    UnaryCall(UnaryCall&&) = delete;
    UnaryCall& operator=(UnaryCall&&) = delete;

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    void Perform() { CallWithRetries(); }

    Response&& ExtractResponse() {
        if (!done_) {
            throw RpcCancelledError{state_.GetCallName(), "UnaryCall"};
        }
        if (!status_.ok()) {
            ugrpc::client::ThrowErrorWithStatus(state_.GetCallName(), std::move(status_));
        }
        return std::move(response_);
    }

    void Abandon() { abandoned_ = true; }

private:
    void CallWithRetries() {
        const auto task_deadline = USERVER_NAMESPACE::server::request::GetTaskInheritedDeadline();
        const int max_attempts = call_options_.GetAttempts();
        state_.GetSpan().AddTag(tracing::kMaxAttempts, max_attempts);

        int attempt = 1;
        RetryBackoff retry_backoff;

        while (!engine::current_task::ShouldCancel()) {
            state_.GetSpan().AddTag(tracing::kAttempts, attempt);
            impl::SetupClientContext(state_, call_options_);

            const bool completed = PerformAttempt();
            if (!completed) {
                break;
            }

            if (status_.ok()) {
                OnDone(status_);
                return;
            }

            if (max_attempts <= attempt || !IsRetryable(status_.error_code())) {
                OnDone(status_);
                return;
            }

            const auto delay = retry_backoff.NextAttemptDelay();
            if (task_deadline.IsReachable() && task_deadline.TimeLeft() <= delay) {
                OnDone(status_);
                return;
            }

            ++attempt;
            engine::InterruptibleSleepFor(delay);
        }

        OnCancelled();
    }

    std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> StartCall() {
        auto call = prepare_unary_call_(state_.GetStub(), &state_.GetClientContext(), request_, &state_.GetQueue());
        call->StartCall();
        return call;
    }

    bool PerformAttempt() {
        RunStartCallHooks();

        auto response_reader = StartCall();

        ugrpc::impl::AsyncMethodInvocation invocation;
        response_reader->Finish(&response_, &status_, invocation.GetCompletionTag());
        const auto wait_status = invocation.Wait();
        if (ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled == wait_status) {
            state_.GetClientContext().TryCancel();
            return false;
        }

        if (status_.ok() && ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError == wait_status) {
            // CompletionQueue returned ok=false. For Client-side Finish ok should always be true.
            // If a GRPC status has been set despite ok=false, or if it has been set by a previous attempt, keep it.
            // Otherwise, propagate ok=false to a failure status.
            status_ = grpc::Status{grpc::StatusCode::INTERNAL, "Client-side Finish CompletionQueue status failed"};
        }

        RunFinishHooks(status_);

        return true;
    }

    void RunStartCallHooks() { impl::RunMiddlewarePipeline(state_, StartCallHooks(ToBaseMessage(&request_))); }

    void RunFinishHooks(const grpc::Status& status) {
        impl::RunMiddlewarePipeline(state_, FinishHooks(status, ToBaseMessage(&response_)));
    }

    void OnDone(const grpc::Status& status) {
        done_ = true;
        impl::HandleCallStatistics(state_, status);
        impl::SetStatusForSpan(state_.GetSpan(), status);
        state_.ResetSpan();
    }

    void OnCancelled() {
        if (abandoned_) {
            impl::SetErrorForSpan(state_.GetSpan(), "Call abandoned");
        } else {
            state_.GetStatsScope().OnCancelled();
            impl::SetErrorForSpan(state_.GetSpan(), "Call cancelled");
        }
        state_.GetStatsScope().Flush();
        state_.ResetSpan();
    }

    CallOptions call_options_;
    CallState state_;
    CallContext context_;

    PrepareUnaryCall prepare_unary_call_;
    const Request& request_;

    Response response_;
    grpc::Status status_;
    bool done_{false};

    std::atomic<bool> abandoned_{false};
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
