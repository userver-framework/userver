#include <userver/ugrpc/client/impl/call_state.hpp>

#include <dynamic_config/variables/USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION.hpp>

#include <userver/tracing/opentelemetry.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <userver/ugrpc/client/impl/call_params.hpp>
#include <userver/ugrpc/client/impl/tracing.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <ugrpc/client/impl/call_options_accessor.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void SetupSpan(std::optional<tracing::InPlaceSpan>& span_holder, std::string_view call_name) {
    UASSERT(!span_holder);
    span_holder.emplace(utils::StrCat("external_grpc/", call_name), utils::impl::SourceLocation::Current());
    auto& span = span_holder->Get();
    span.DetachFromCoroStack();
}

void AddTracingMetadata(grpc::ClientContext& client_context, const tracing::Span& span) {
    if (const auto span_id = span.GetSpanIdForChildLogs()) {
        client_context.AddMetadata(ugrpc::impl::kXYaTraceId, ugrpc::impl::ToGrpcString(span.GetTraceId()));
        client_context.AddMetadata(ugrpc::impl::kXYaSpanId, ugrpc::impl::ToGrpcString(*span_id));
        client_context.AddMetadata(ugrpc::impl::kXYaRequestId, ugrpc::impl::ToGrpcString(span.GetLink()));

        constexpr std::string_view kDefaultOtelTraceFlags = "01";
        auto traceparent =
            tracing::opentelemetry::BuildTraceParentHeader(span.GetTraceId(), *span_id, kDefaultOtelTraceFlags);

        if (!traceparent.has_value()) {
            LOG_LIMITED_DEBUG("Cannot build opentelemetry traceparent header ({})", traceparent.error());
            return;
        }
        client_context.AddMetadata(ugrpc::impl::kTraceParent, ugrpc::impl::ToGrpcString(traceparent.value()));
    }
}

}  // namespace

RpcConfigValues::RpcConfigValues(const dynamic_config::Snapshot& config)
    : enforce_task_deadline(config[::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION]) {}

CallState::CallState(CallParams&& params, CallKind call_kind)
    : stub_(std::move(params.stub)),
      client_name_(params.client_name),
      call_name_(std::move(params.call_name)),
      stats_scope_(params.statistics),
      queue_(params.queue),
      config_values_(params.config),
      middleware_pipeline_(params.middlewares),
      testsuite_grpc_(params.testsuite_grpc),
      call_kind_(call_kind) {
    UINVARIANT(!client_name_.empty(), "client name should not be empty");

    SetupSpan(span_, call_name_.Get());
}

StubHandle& CallState::GetStub() noexcept { return stub_; }

void CallState::SetClientContext(std::unique_ptr<grpc::ClientContext> client_context) noexcept {
    client_context_ = std::move(client_context);
}

const grpc::ClientContext& CallState::GetClientContext() const noexcept {
    UASSERT(client_context_);
    return *client_context_;
}

grpc::ClientContext& CallState::GetClientContext() noexcept {
    UASSERT(client_context_);
    return *client_context_;
}

grpc::CompletionQueue& CallState::GetQueue() const noexcept { return queue_; }

const RpcConfigValues& CallState::GetConfigValues() const noexcept { return config_values_; }

const MiddlewarePipeline& CallState::GetMiddlewarePipeline() const noexcept { return middleware_pipeline_; }

const testsuite::GrpcControl& CallState::GetTestsuiteControl() const noexcept { return testsuite_grpc_; }

std::string_view CallState::GetCallName() const noexcept { return call_name_.Get(); }

std::string_view CallState::GetClientName() const noexcept { return client_name_; }

tracing::Span& CallState::GetSpan() noexcept {
    UASSERT(span_);
    return span_->Get();
}

CallKind CallState::GetCallKind() const noexcept { return call_kind_; }

void CallState::ResetSpan() noexcept {
    UASSERT(span_);
    span_.reset();
}

ugrpc::impl::RpcStatisticsScope& CallState::GetStatsScope() noexcept { return stats_scope_; }

void CallState::SetDeadlinePropagated() noexcept {
    stats_scope_.OnDeadlinePropagated();
    is_deadline_propagated_ = true;
}

bool CallState::IsDeadlinePropagated() const noexcept { return is_deadline_propagated_; }

grpc::Status& CallState::GetStatus() noexcept { return status_; }

void CallState::Commit() noexcept { committed_.store(true, std::memory_order_release); }

grpc::ClientContext& CallState::GetClientContextCommitted() {
    UINVARIANT(committed_, "Call state should be committed");
    UINVARIANT(client_context_, "GetClientContext should not be called on cancelled RPC");
    return *client_context_;
}

StreamingCallState::StreamingCallState(CallParams&& params, CallKind call_kind)
    : CallState(std::move(params), call_kind) {
    SetupClientContext(*this, params.call_options);
    Commit();
}

StreamingCallState::~StreamingCallState() noexcept {
    invocation_.reset();

    if (!IsFinished()) {
        GetClientContext().TryCancel();

        SetErrorForSpan(GetSpan(), "Abandoned");
        ResetSpan();
    }
}

void StreamingCallState::SetWritesFinished() noexcept {
    UINVARIANT(!writes_finished_, "Writes already finished");
    writes_finished_ = true;
}

bool StreamingCallState::AreWritesFinished() const noexcept { return writes_finished_; }

void StreamingCallState::SetFinished() noexcept {
    UINVARIANT(!is_finished_, "Tried to finish an already finished call");
    is_finished_ = true;
}

bool StreamingCallState::IsFinished() const noexcept { return is_finished_; }

void StreamingCallState::EmplaceAsyncMethodInvocation() {
    UINVARIANT(!invocation_.has_value(), "Another method is already running for this RPC concurrently");
    invocation_.emplace();
}

ugrpc::impl::AsyncMethodInvocation& StreamingCallState::GetAsyncMethodInvocation() noexcept {
    UASSERT(invocation_.has_value());
    return *invocation_;
}

std::unique_lock<engine::SingleWaitingTaskMutex> StreamingCallState::TakeMutexIfBidirectional() noexcept {
    if (GetCallKind() == impl::CallKind::kBidirectionalStream) {
        // Analogy as 'ugrpc::server::impl::ResponseBase::TakeMutexIfBidirectional'
        return std::unique_lock(bidirectional_mutex_);
    }
    return {};
}

StreamingCallState::AsyncMethodInvocationGuard::AsyncMethodInvocationGuard(StreamingCallState& state) noexcept
    : state_(state) {
    UASSERT(state_.invocation_.has_value());
}

StreamingCallState::AsyncMethodInvocationGuard::~AsyncMethodInvocationGuard() noexcept {
    UASSERT(state_.invocation_.has_value());
    if (!disarm_) {
        state_.invocation_.reset();
    }
}

bool IsReadAvailable(const StreamingCallState& state) noexcept { return !state.IsFinished(); }

bool IsWriteAvailable(const StreamingCallState& state) noexcept { return !state.AreWritesFinished(); }

bool IsWriteAndCheckAvailable(const StreamingCallState& state) noexcept {
    return !state.AreWritesFinished() && !state.IsFinished();
}

void SetupClientContext(CallState& state, const CallOptions& call_options) {
    auto client_context = CallOptionsAccessor::CreateClientContext(call_options);
    AddTracingMetadata(*client_context, state.GetSpan());
    state.SetClientContext(std::move(client_context));
}

void HandleCallStatistics(CallState& state, const grpc::Status& status) noexcept {
    auto& stats = state.GetStatsScope();
    stats.OnExplicitFinish(status.error_code());
    if (grpc::StatusCode::DEADLINE_EXCEEDED == status.error_code() && state.IsDeadlinePropagated()) {
        stats.OnCancelledByDeadlinePropagation();
    }
    stats.Flush();
}

void RunMiddlewarePipeline(CallState& state, const MiddlewareHooks& hooks) {
    MiddlewareCallContext middleware_call_context{state};
    state.GetMiddlewarePipeline().Run(hooks, middleware_call_context);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
