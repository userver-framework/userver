#pragma once

#include <optional>
#include <string_view>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/status.h>

#include <userver/dynamic_config/fwd.hpp>
#include <userver/engine/single_waiting_task_mutex.hpp>
#include <userver/tracing/in_place_span.hpp>

#include <userver/ugrpc/client/impl/async_method_invocation.hpp>
#include <userver/ugrpc/client/impl/call_kind.hpp>
#include <userver/ugrpc/client/impl/stub_handle.hpp>
#include <userver/ugrpc/client/middlewares/fwd.hpp>
#include <userver/ugrpc/impl/async_method_invocation.hpp>
#include <userver/ugrpc/impl/maybe_owned_string.hpp>
#include <userver/ugrpc/impl/statistics_scope.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

struct CallParams;

struct RpcConfigValues final {
    explicit RpcConfigValues(const dynamic_config::Snapshot& config);

    bool enforce_task_deadline;
};

class CallState {
public:
    CallState(CallParams&&, CallKind);

    ~CallState() noexcept = default;

    CallState(CallState&&) noexcept = delete;
    CallState& operator=(CallState&&) noexcept = delete;

    StubHandle& GetStub() noexcept;

    const grpc::ClientContext& GetClientContext() const noexcept;

    grpc::ClientContext& GetClientContext() noexcept;

    std::string_view GetCallName() const noexcept;

    std::string_view GetClientName() const noexcept;

    tracing::Span& GetSpan() noexcept;

    grpc::CompletionQueue& GetQueue() const noexcept;

    const RpcConfigValues& GetConfigValues() const noexcept;

    const Middlewares& GetMiddlewares() const noexcept;

    CallKind GetCallKind() const noexcept;

    void ResetSpan() noexcept;

    ugrpc::impl::RpcStatisticsScope& GetStatsScope() noexcept;

    bool IsDeadlinePropagated() const noexcept;

    void SetDeadlinePropagated() noexcept;

    grpc::Status& GetStatus() noexcept;

private:
    StubHandle stub_;

    std::unique_ptr<grpc::ClientContext> client_context_;

    std::string client_name_;
    ugrpc::impl::MaybeOwnedString call_name_;

    bool is_deadline_propagated_{false};

    std::optional<tracing::InPlaceSpan> span_;

    ugrpc::impl::RpcStatisticsScope stats_scope_;

    grpc::CompletionQueue& queue_;

    RpcConfigValues config_values_;

    const Middlewares& middlewares_;

    CallKind call_kind_{};

    grpc::Status status_;
};

class UnaryCallState final : public CallState {
public:
    explicit UnaryCallState(CallParams&& params) : CallState(std::move(params), CallKind::kUnaryCall) {}

    ~UnaryCallState() noexcept;

    UnaryCallState(UnaryCallState&&) noexcept = delete;
    UnaryCallState& operator=(UnaryCallState&&) noexcept = delete;

    bool IsFinishProcessed() const noexcept;
    void SetFinishProcessed() noexcept;

    bool IsStatusExtracted() const noexcept;
    void SetStatusExtracted() noexcept;

    void EmplaceFinishAsyncMethodInvocation();

    FinishAsyncMethodInvocation& GetFinishAsyncMethodInvocation() noexcept;

private:
    bool finish_processed_{false};

    bool status_extracted_{false};

    // In unary call the call is finished as soon as grpc core
    // gives us back a response - so for unary call
    // We use FinishAsyncMethodInvocation that will correctly close all our
    // tracing::Span objects and account everything in statistics.
    std::optional<FinishAsyncMethodInvocation> invocation_;
};

class StreamingCallState final : public CallState {
public:
    StreamingCallState(CallParams&& params, CallKind call_kind) : CallState(std::move(params), call_kind) {}

    ~StreamingCallState() noexcept;

    StreamingCallState(StreamingCallState&&) noexcept = delete;
    StreamingCallState& operator=(StreamingCallState&&) noexcept = delete;

    void SetWritesFinished() noexcept;

    bool AreWritesFinished() const noexcept;

    void SetFinished() noexcept;

    bool IsFinished() const noexcept;

    // please read comments for 'invocation_' private member on why
    // we use different invocation types
    void EmplaceAsyncMethodInvocation();

    // please read comments for 'invocation_' private member on why
    // we use different invocation types
    ugrpc::impl::AsyncMethodInvocation& GetAsyncMethodInvocation() noexcept;

    [[nodiscard]] std::unique_lock<engine::SingleWaitingTaskMutex> TakeMutexIfBidirectional() noexcept;

    class AsyncMethodInvocationGuard {
    public:
        AsyncMethodInvocationGuard(StreamingCallState& state) noexcept;

        ~AsyncMethodInvocationGuard() noexcept;

        AsyncMethodInvocationGuard(const AsyncMethodInvocationGuard&) = delete;
        AsyncMethodInvocationGuard(AsyncMethodInvocationGuard&&) = delete;

        void Disarm() noexcept { disarm_ = true; }

    private:
        StreamingCallState& state_;
        bool disarm_{false};
    };

private:
    bool writes_finished_{false};

    bool is_finished_{false};

    engine::SingleWaitingTaskMutex bidirectional_mutex_;

    // We use FinishAsyncMethodInvocation that will correctly close all our
    // tracing::Span objects and account everything in statistics.
    // and AsyncMethodInvocation for every intermediate
    // Read* call, because we don't need to close span and/or account stats
    // when finishing Read* call.
    //
    // This field must go after other fields to be destroyed first!
    std::optional<ugrpc::impl::AsyncMethodInvocation> invocation_;
};

bool IsReadAvailable(const StreamingCallState&) noexcept;

bool IsWriteAvailable(const StreamingCallState&) noexcept;

bool IsWriteAndCheckAvailable(const StreamingCallState&) noexcept;

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
