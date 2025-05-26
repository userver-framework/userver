#pragma once

#include <memory>
#include <optional>
#include <string_view>
#include <variant>

#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/status.h>

#include <userver/dynamic_config/fwd.hpp>
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
using ugrpc::impl::AsyncMethodInvocation;

struct RpcConfigValues final {
    explicit RpcConfigValues(const dynamic_config::Snapshot& config);

    bool enforce_task_deadline;
};

class CallState final {
public:
    CallState(CallParams&&, CallKind);

    CallState(CallState&&) noexcept = delete;
    CallState& operator=(CallState&&) noexcept = delete;
    ~CallState() noexcept;

    StubHandle& GetStub() noexcept;

    const grpc::ClientContext& GetContext() const noexcept;

    grpc::ClientContext& GetContext() noexcept;

    std::string_view GetCallName() const noexcept;

    std::string_view GetClientName() const noexcept;

    tracing::Span& GetSpan() noexcept;

    grpc::CompletionQueue& GetQueue() const noexcept;

    const RpcConfigValues& GetConfigValues() const noexcept;

    const Middlewares& GetMiddlewares() const noexcept;

    CallKind GetCallKind() const noexcept;

    void ResetSpan() noexcept;

    ugrpc::impl::RpcStatisticsScope& GetStatsScope() noexcept;

    void SetWritesFinished() noexcept;

    bool AreWritesFinished() const noexcept;

    void SetFinished() noexcept;

    bool IsFinished() const noexcept;

    bool IsDeadlinePropagated() const noexcept;

    void SetDeadlinePropagated() noexcept;

    bool IsReadAvailable() const noexcept;

    bool IsWriteAvailable() const noexcept;

    bool IsWriteAndCheckAvailable() const noexcept;

    // please read comments for 'invocation_' private member on why
    // we use two different invocation types
    void EmplaceAsyncMethodInvocation();

    // please read comments for 'invocation_' private member on why
    // we use two different invocation types
    void EmplaceFinishAsyncMethodInvocation();

    // please read comments for 'invocation_' private member on why
    // we use two different invocation types
    AsyncMethodInvocation& GetAsyncMethodInvocation() noexcept;

    // please read comments for 'invocation_' private member on why
    // we use two different invocation types
    FinishAsyncMethodInvocation& GetFinishAsyncMethodInvocation() noexcept;

    // These are for asserts and invariants. Do NOT branch actual code
    // based on these two functions. Branching based on these two functions
    // is considered UB, no diagnostics required.
    bool HoldsAsyncMethodInvocationDebug() noexcept;
    bool HoldsFinishAsyncMethodInvocationDebug() noexcept;

    bool IsFinishProcessed() const noexcept;
    void SetFinishProcessed() noexcept;

    bool IsStatusExtracted() const noexcept;
    void SetStatusExtracted() noexcept;

    grpc::Status& GetStatus() noexcept;

    class AsyncMethodInvocationGuard {
    public:
        AsyncMethodInvocationGuard(CallState& state) noexcept;
        AsyncMethodInvocationGuard(const AsyncMethodInvocationGuard&) = delete;
        AsyncMethodInvocationGuard(AsyncMethodInvocationGuard&&) = delete;
        ~AsyncMethodInvocationGuard() noexcept;

        void Disarm() noexcept { disarm_ = true; }

    private:
        CallState& state_;
        bool disarm_{false};
    };

private:
    StubHandle stub_;
    std::unique_ptr<grpc::ClientContext> context_;
    std::string client_name_;
    ugrpc::impl::MaybeOwnedString call_name_;
    bool writes_finished_{false};
    bool is_finished_{false};
    bool is_deadline_propagated_{false};

    std::optional<tracing::InPlaceSpan> span_;
    ugrpc::impl::RpcStatisticsScope stats_scope_;
    grpc::CompletionQueue& queue_;
    RpcConfigValues config_values_;
    const Middlewares& mws_;

    CallKind call_kind_{};

    grpc::Status status_;
    bool finish_processed_{false};
    bool status_extracted_{false};

    // This data is common for all types of grpc calls - unary and streaming
    // However, in unary call the call is finished as soon as grpc core
    // gives us back a response - so for unary call we use
    // FinishAsyncMethodInvocation that will correctly close all our
    // tracing::Span objects and account everything in statistics.
    // In stream response, we use AsyncMethodInvocation for every intermediate
    // Read* call, because we don't need to close span and/or account stats
    // when finishing Read* call.
    //
    // This field must go after other fields to be destroyed first!
    std::variant<std::monostate, AsyncMethodInvocation, FinishAsyncMethodInvocation> invocation_;
};

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
