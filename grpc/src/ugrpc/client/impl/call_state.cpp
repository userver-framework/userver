#include <userver/ugrpc/client/impl/call_state.hpp>

#include <dynamic_config/variables/USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION.hpp>

#include <fmt/format.h>

#include <string>

#include <userver/http/url.hpp>
#include <userver/tracing/manager.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <userver/ugrpc/client/impl/call_params.hpp>
#include <userver/ugrpc/client/impl/tracing.hpp>
#include <userver/ugrpc/client/middlewares/base.hpp>
#include <userver/ugrpc/impl/static_service_metadata.hpp>
#include <userver/ugrpc/impl/to_string.hpp>

#include <ugrpc/client/impl/call_options_accessor.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void ConstructSpan(std::optional<tracing::InPlaceSpan>& span_storage, std::string_view call_name) {
    UASSERT(!span_storage.has_value());
    span_storage.emplace(std::string{call_name}, utils::impl::SourceLocation::Current());
    span_storage->Get().DetachFromCoroStack();
}

void AddServiceMethodTags(
    tracing::Span& span,
    std::string_view endpoint,
    std::string_view service_name,
    std::string_view method_name
) {
    span.AddNonInheritableTag(tracing::kRpcSystem, "grpc");
    span.AddNonInheritableTag(tracing::kServerAddress, USERVER_NAMESPACE::http::ExtractHostname(endpoint));
    span.AddNonInheritableTag(tracing::kRpcService, std::string{service_name});
    span.AddNonInheritableTag(tracing::kRpcMethod, std::string{method_name});
}

void AddTracingMetadata(grpc::ClientContext& client_context, const tracing::Span& span) {
    if (const auto span_id = span.GetSpanIdForChildLogs()) {
        client_context.AddMetadata(ugrpc::impl::kXYaTraceId, ugrpc::impl::ToGrpcString(span.GetTraceId()));
        client_context.AddMetadata(ugrpc::impl::kXYaSpanId, ugrpc::impl::ToGrpcString(*span_id));
        client_context.AddMetadata(ugrpc::impl::kXYaRequestId, ugrpc::impl::ToGrpcString(span.GetLink()));

        const auto flags_hex = fmt::format("{:02x}", static_cast<unsigned>(tracing::GetInheritedOtelTraceFlags()));
        auto traceparent = tracing::opentelemetry::BuildTraceParentHeader(span.GetTraceId(), *span_id, flags_hex);

        if (!traceparent.has_value()) {
            LOG_LIMITED_DEBUG("Cannot build opentelemetry traceparent header ({})", traceparent.error());
            return;
        }
        client_context.AddMetadata(ugrpc::impl::kTraceParent, ugrpc::impl::ToGrpcString(traceparent.value()));

        const auto tracestate = tracing::GetInheritedOtelTraceState();
        if (!tracestate.empty()) {
            client_context.AddMetadata(ugrpc::impl::kTraceState, ugrpc::impl::ToGrpcString(tracestate));
        }
    }
}

}  // namespace

RpcConfigValues::RpcConfigValues(const dynamic_config::Snapshot& config)
    : enforce_task_deadline(config[::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION])
{}

CallState::CallState(CallParams&& params)
    : method_stubs_(std::move(params.method_stubs)),
      client_name_(params.client_name),
      rpc_type_(params.rpc_type),
      method_path_(std::move(params.method_path)),
      stats_scope_(params.statistics),
      queue_(params.queue),
      config_values_(params.config),
      middleware_pipeline_(params.middlewares),
      testsuite_grpc_(params.testsuite_grpc),
      retry_limiter_(params.retry_limiter)
{
    UINVARIANT(!client_name_.empty(), "client name should not be empty");

    ConstructSpan(span_, GetCallName());
    const auto method_name_parts = ugrpc::impl::ParseMethodName(GetMethodPath());
    AddServiceMethodTags(span_->Get(), params.endpoint, method_name_parts.service_name, method_name_parts.method_name);
}

ugrpc::impl::StubAny& CallState::GetStub() const noexcept { return method_stubs_.GetStub(); }

grpc::GenericStub& CallState::GetGenericStub() const noexcept { return method_stubs_.GetGenericStub(); }

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

std::string_view CallState::GetClientName() const noexcept { return client_name_; }

std::string_view CallState::GetCallName() const noexcept { return ugrpc::impl::GetCallName(GetMethodPath()); }

std::string_view CallState::GetEndpoint() const noexcept { return method_stubs_.GetEndpoint(); }

AuthType CallState::GetAuthType() const noexcept { return method_stubs_.GetAuthType(); }

RpcType CallState::GetRpcType() const noexcept { return rpc_type_; }

const grpc::string& CallState::GetMethodPath() const noexcept { return method_path_; }

tracing::Span& CallState::GetSpan() noexcept {
    UASSERT(span_);
    return span_->Get();
}

grpc::CompletionQueue& CallState::GetQueue() const noexcept { return queue_; }

const RpcConfigValues& CallState::GetConfigValues() const noexcept { return config_values_; }

const MiddlewarePipeline& CallState::GetMiddlewarePipeline() const noexcept { return middleware_pipeline_; }

const testsuite::GrpcControl& CallState::GetTestsuiteControl() const noexcept { return testsuite_grpc_; }

RetryLimiter* CallState::GetRetryLimiter() const noexcept { return retry_limiter_; }

ugrpc::impl::RpcStatisticsScope& CallState::GetStatsScope() noexcept { return stats_scope_; }

bool CallState::IsDeadlinePropagated() const noexcept { return is_deadline_propagated_; }

void CallState::SetDeadlinePropagated() noexcept {
    stats_scope_.OnDeadlinePropagated();
    is_deadline_propagated_ = true;
}

void CallState::Commit() noexcept { committed_.store(true, std::memory_order_release); }

grpc::ClientContext& CallState::GetClientContextCommitted() {
    UINVARIANT(committed_.load(std::memory_order_acquire), "Call state should be committed");
    UINVARIANT(client_context_, "GetClientContext should not be called on cancelled RPC");
    return *client_context_;
}

void CallState::ResetSpan() noexcept {
    UASSERT(span_);
    span_.reset();
}

StreamingCallState::StreamingCallState(CallParams&& params)
    : CallState(std::move(params))
{
    // CallState constructor does not consume call_options.
    // NOLINTNEXTLINE(bugprone-use-after-move)
    SetupClientContext(*this, params.call_options, /*attempt*/ 1);
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
    if (GetRpcType() == RpcType::kBidiStreaming) {
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

void SetupClientContext(CallState& state, const CallOptions& call_options, int attempt) {
    auto client_context = CallOptionsAccessor::CreateClientContext(call_options);

    if (1 < attempt) {
        const auto prev_attempts = ugrpc::impl::ToGrpcString(std::to_string(attempt - 1));
        client_context->AddMetadata(ugrpc::impl::kXPrevAttempts, prev_attempts);
    }

    AddTracingMetadata(*client_context, state.GetSpan());

    state.SetClientContext(std::move(client_context));
}

void RunMiddlewarePipeline(CallState& state, const MiddlewareHooks& hooks) {
    MiddlewareCallContext middleware_call_context{state};
    state.GetMiddlewarePipeline().Run(hooks, middleware_call_context);
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
