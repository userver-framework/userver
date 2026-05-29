#pragma once

#include <google/protobuf/message.h>
#include <grpcpp/support/async_unary_call.h>

#include <userver/engine/sleep.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/impl/internal_tag.hpp>

#include <userver/ugrpc/client/call_context.hpp>
#include <userver/ugrpc/client/client_qos.hpp>
#include <userver/ugrpc/client/completion_status.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>
#include <userver/ugrpc/client/impl/call_params.hpp>
#include <userver/ugrpc/client/impl/call_state.hpp>
#include <userver/ugrpc/client/impl/deadline_propagation_detect.hpp>
#include <userver/ugrpc/client/impl/middleware_hooks.hpp>
#include <userver/ugrpc/client/impl/prepare_async_call.hpp>
#include <userver/ugrpc/client/impl/retry_backoff.hpp>
#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/impl/status_utils.hpp>
#include <userver/ugrpc/status_codes.hpp>
#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

inline bool IsRetryable(const CompletionStatus& completion_status) noexcept {
    return completion_status.has_value() && ugrpc::IsRetryable(completion_status.value().error_code());
}

// Returns true if it's okay to send a retry
inline bool AccountCompletion(RetryLimiter& retry_limiter, const CompletionStatus& completion_status) {
    retry_limiter.AccountCompletion(completion_status);
    return retry_limiter.CanRetry();
}

void AccountStatistics(ugrpc::impl::RpcStatisticsScope& stats, CompletionStatus completion_status) noexcept;

void SetStatusForSpan(tracing::Span& span, CompletionStatus completion_status) noexcept;

[[noreturn]] void ThrowSpecialCaseCompletionError(
    std::string_view call_name,
    SpecialCaseCompletionType special_case_completion_type
);

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

    Response Perform() {
        const utils::FastScopeGuard span_scope([this]() noexcept { state_.ResetSpan(); });

        auto completion_status = InterceptCall();

        impl::AccountStatistics(state_.GetStatsScope(), completion_status);

        impl::SetStatusForSpan(state_.GetSpan(), completion_status);

        return HandleCompletion(completion_status);
    }

    void Abandon() noexcept {
        // If we receive a cancellation due to abandonment, we are guaranteed to receive the value `abandoned_ == true`
        // due to the engine's acquire-release guarantees.
        // If the request is successfully processed and not dropped, it will always be `abandoned_ == false`.
        // If there was a race condition between abandonment and notification, `abandoned_ == true` may or may not leak,
        // but if it does, we don't want to do any unnecessary work.
        abandoned_.store(true, std::memory_order_relaxed);
    }

private:
    CompletionStatus InterceptCall() {
        const utils::FastScopeGuard commit_state_guard([this]() noexcept { state_.Commit(); });

        const auto inherited_deadline = USERVER_NAMESPACE::server::request::GetTaskInheritedDeadline();
        const auto deadline = std::min(call_options_.GetDeadline(), inherited_deadline);
        const int max_attempts = call_options_.GetAttempts();
        state_.GetSpan().AddTag(tracing::kMaxAttempts, max_attempts);

        int attempt = 0;
        RetryBackoff retry_backoff;

        while (!engine::current_task::ShouldCancel()) {
            ++attempt;
            state_.GetSpan().AddTag(tracing::kAttempts, attempt);
            impl::SetupClientContext(state_, call_options_, attempt);

            RunStartCallHooks();

            auto completion_status = PerformAttempt();

            RunFinishHooks(completion_status);

            auto* retry_limiter = state_.GetRetryLimiter();
            const bool retry_allowed = !retry_limiter || impl::AccountCompletion(*retry_limiter, completion_status);

            if (max_attempts <= attempt || !retry_allowed || !impl::IsRetryable(completion_status)) {
                return completion_status;
            }

            const auto delay = retry_backoff.NextAttemptDelay();
            if (deadline.IsReachable() && deadline.TimeLeft() <= delay) {
                if (deadline == inherited_deadline) {
                    USERVER_NAMESPACE::server::request::MarkTaskInheritedDeadlineExpired();
                }
                return completion_status;
            }

            engine::InterruptibleSleepFor(delay);
        }

        if (abandoned_.load(std::memory_order_relaxed)) {
            return utils::unexpected{SpecialCaseCompletionType::kAbandoned};
        }

        if (impl::IsTaskCancelledByDeadlinePropagation()) {
            return utils::unexpected{SpecialCaseCompletionType::kTimeoutDeadlinePropagated};
        }

        return utils::unexpected{SpecialCaseCompletionType::kCancelled};
    }

    CompletionStatus PerformAttempt() {
        // Awaits previous attempt completion, if any.
        // The last attempt completion is awaited in the destructor.
        finish_invocation_.emplace();

        call_ = StartCall();
        call_->Finish(&response_, &status_, finish_invocation_->GetCompletionTag());
        const auto wait_status = finish_invocation_->Wait();

        if (ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled == wait_status) {
            // If the user cancelled the request, notify grpcpp as soon as possible
            // (we still must wait for the operation to complete, but grpcpp should finish fast)
            state_.GetClientContext().TryCancel();
        }

        if (abandoned_.load(std::memory_order_relaxed)) {
            return utils::unexpected{SpecialCaseCompletionType::kAbandoned};
        }

        switch (wait_status) {
            case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kOk:
                if (impl::IsRequestCancelledByDeadlinePropagation(status_, state_)) {
                    return utils::unexpected{SpecialCaseCompletionType::kTimeoutDeadlinePropagated};
                }
                ugrpc::impl::ClampStatusCodeToValidRange(status_);
                return std::move(status_);

            case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kError:
                // CompletionQueue returned ok=false. For Client-side Finish ok should always be true.
                // RPC has finished in abnormal manner.
                return utils::unexpected{SpecialCaseCompletionType::kNetworkError};

            case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled:
                if (impl::IsTaskCancelledByDeadlinePropagation()) {
                    return utils::unexpected{SpecialCaseCompletionType::kTimeoutDeadlinePropagated};
                }
                return utils::unexpected{SpecialCaseCompletionType::kCancelled};

            case ugrpc::impl::AsyncMethodInvocation::WaitStatus::kDeadline:
                UINVARIANT(false, "Unexpected 'AsyncMethodInvocation::WaitStatus'");
        }
        UINVARIANT(false, "unreachable");  // (gcc 11): control reaches end of non-void function
    }

    std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> StartCall() {
        auto call = prepare_unary_call_(state_.GetStub(), &state_.GetClientContext(), request_, &state_.GetQueue());
        call->StartCall();
        return call;
    }

    void RunStartCallHooks() {
        impl::RunMiddlewarePipeline(state_, MiddlewareHooks::StartCallHooks(ToBaseMessage(&request_)));
    }

    void RunFinishHooks(const CompletionStatus& completion_status) {
        impl::RunMiddlewarePipeline(
            state_,
            MiddlewareHooks::FinishHooks(
                completion_status,
                completion_status.has_value() && completion_status.value().ok() ? ToBaseMessage(&response_) : nullptr
            )
        );
    }

    Response HandleCompletion(CompletionStatus& completion_status) {
        if (!completion_status.has_value()) {
            impl::ThrowSpecialCaseCompletionError(state_.GetCallName(), completion_status.error());
        }

        auto& status = completion_status.value();
        if (!status.ok()) {
            ugrpc::client::ThrowErrorWithStatus(state_.GetCallName(), std::move(status));
        }

        return std::move(response_);
    }

    CallOptions call_options_;
    CallState state_;
    CallContext context_;

    PrepareUnaryCall prepare_unary_call_;
    const Request& request_;

    std::unique_ptr<grpc::ClientAsyncResponseReader<Response>> call_;
    Response response_;
    grpc::Status status_;
    // Must go after `call_`, `response_` and `status_` to await `Finish` completion before destroying vars it needs.
    std::optional<ugrpc::impl::AsyncMethodInvocation> finish_invocation_;

    std::atomic<bool> abandoned_{false};
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
