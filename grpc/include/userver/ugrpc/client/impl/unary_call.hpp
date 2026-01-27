#pragma once

#include <fmt/format.h>
#include <google/protobuf/message.h>
#include <grpcpp/support/async_unary_call.h>

#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/fast_scope_guard.hpp>
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
#include <userver/ugrpc/impl/status_utils.hpp>
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
          state_{std::move(params)},
          context_{utils::impl::InternalTag{}, state_},
          prepare_unary_call_{std::move(prepare_unary_call)},
          request_{request}
    {}

    ~UnaryCall() = default;

    UnaryCall(UnaryCall&&) = delete;
    UnaryCall& operator=(UnaryCall&&) = delete;

    CallContext& GetContext() noexcept { return context_; }
    const CallContext& GetContext() const noexcept { return context_; }

    void Perform() { CallWithRetries(); }

    Response&& ExtractResponse() {
        if (inherited_deadline_reached_) {
            USERVER_NAMESPACE::server::request::MarkTaskInheritedDeadlineExpired();
        }
        if (interrupted_) {
            throw RpcInterruptedError(state_.GetCallName(), "UnaryCall");
        }
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
        const utils::FastScopeGuard commit_state_guard([this]() noexcept { state_.Commit(); });

        const auto inherited_deadline = USERVER_NAMESPACE::server::request::GetTaskInheritedDeadline();
        const auto deadline = std::min(call_options_.GetDeadline(), inherited_deadline);
        const int max_attempts = call_options_.GetAttempts();
        state_.GetSpan().AddTag(tracing::kMaxAttempts, max_attempts);

        int attempt = 1;
        RetryBackoff retry_backoff;

        while (!engine::current_task::ShouldCancel()) {
            state_.GetSpan().AddTag(tracing::kAttempts, attempt);
            impl::SetupClientContext(state_, call_options_);

            const auto completion_status = PerformAttempt();
            if (AttemptCompletionStatus::kCancelled == completion_status) {
                break;
            }

            if (AttemptCompletionStatus::kError == completion_status) {
                OnInterrupted();
                return;
            }

            UINVARIANT(
                AttemptCompletionStatus::kOk == completion_status,
                "Status becomes available for successfully completed attempt only"
            );

            if (status_.ok()) {
                OnDone(status_);
                return;
            }

            if (max_attempts <= attempt || !IsRetryable(status_.error_code())) {
                OnDone(status_);
                return;
            }

            const auto delay = retry_backoff.NextAttemptDelay();
            if (deadline.IsReachable() && deadline.TimeLeft() <= delay) {
                if (deadline == inherited_deadline) {
                    inherited_deadline_reached_ = true;
                }
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

    enum class AttemptCompletionStatus {
        kOk,
        kError,
        kCancelled,
    };
    AttemptCompletionStatus PerformAttempt() {
        RunStartCallHooks();

        auto response_reader = StartCall();

        ugrpc::impl::AsyncMethodInvocation invocation;
        response_reader->Finish(&response_, &status_, invocation.GetCompletionTag());
        const auto wait_status = invocation.Wait();
        if (ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled == wait_status) {
            state_.GetClientContext().TryCancel();
            return AttemptCompletionStatus::kCancelled;
        }

        if (ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError == wait_status) {
            // CompletionQueue returned ok=false. For Client-side Finish ok should always be true.
            // It signifies that RPC has interrupted in abnormal manner.
            // Do not attempt further operations on the RPC.
            return AttemptCompletionStatus::kError;
        }

        ugrpc::impl::ClampStatusCodeToValidRange(status_);

        RunFinishHooks(status_);

        return AttemptCompletionStatus::kOk;
    }

    void RunStartCallHooks() {
        impl::RunMiddlewarePipeline(state_, MiddlewareHooks::StartCallHooks(ToBaseMessage(&request_)));
    }

    void RunFinishHooks(const grpc::Status& status) {
        impl::RunMiddlewarePipeline(
            state_,
            MiddlewareHooks::FinishHooks(status, status.ok() ? ToBaseMessage(&response_) : nullptr)
        );
    }

    void OnDone(const grpc::Status& status) {
        done_ = true;
        impl::HandleCallStatistics(state_, status);
        impl::SetStatusForSpan(state_.GetSpan(), status);
        state_.ResetSpan();
    }

    void OnInterrupted() {
        interrupted_ = true;
        std::string error_message = "Call interrupted, Network error";
        const auto debug_error_string = state_.GetClientContext().debug_error_string();
        if (!debug_error_string.empty()) {
            error_message += fmt::format("\nAdditional GRPC error information: {}", debug_error_string);
        }
        impl::SetErrorForSpan(state_.GetSpan(), error_message);
        state_.GetStatsScope().OnNetworkError();
        state_.GetStatsScope().Flush();
        state_.ResetSpan();
    }

    void OnCancelled() {
        if (abandoned_) {
            impl::SetErrorForSpan(state_.GetSpan(), "Call abandoned");
        } else {
            impl::SetErrorForSpan(state_.GetSpan(), "Call cancelled");
            state_.GetStatsScope().OnCancelled();
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
    bool interrupted_{false};
    bool inherited_deadline_reached_{false};

    std::atomic<bool> abandoned_{false};
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
