#include <userver/ugrpc/client/impl/call_state.hpp>

#include <dynamic_config/variables/USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION.hpp>

#include <userver/utils/assert.hpp>

#include <userver/ugrpc/client/impl/call_params.hpp>

#include <ugrpc/client/impl/call_options_accessor.hpp>
#include <ugrpc/client/impl/tracing.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

RpcConfigValues::RpcConfigValues(const dynamic_config::Snapshot& config)
    : enforce_task_deadline(config[::dynamic_config::USERVER_GRPC_CLIENT_ENABLE_DEADLINE_PROPAGATION]) {}

CallState::CallState(CallParams&& params, CallKind call_kind)
    : stub_(std::move(params.stub)),
      client_name_(params.client_name),
      call_name_(std::move(params.call_name)),
      stats_scope_(params.statistics),
      queue_(params.queue),
      config_values_(params.config),
      middlewares_(params.middlewares),
      call_kind_(call_kind) {
    UINVARIANT(!client_name_.empty(), "client name should not be empty");
    SetupSpan(span_, call_name_.Get());

    client_context_ = CallOptionsAccessor::CreateClientContext(params.call_options);
    AddTracingMetadata(*client_context_, GetSpan());
}

StubHandle& CallState::GetStub() noexcept { return stub_; }

const grpc::ClientContext& CallState::GetClientContext() const noexcept { return *client_context_; }

grpc::ClientContext& CallState::GetClientContext() noexcept { return *client_context_; }

grpc::CompletionQueue& CallState::GetQueue() const noexcept { return queue_; }

const RpcConfigValues& CallState::GetConfigValues() const noexcept { return config_values_; }

const Middlewares& CallState::GetMiddlewares() const noexcept { return middlewares_; }

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

UnaryCallState::~UnaryCallState() noexcept { invocation_.reset(); }

bool UnaryCallState::IsFinishProcessed() const noexcept { return finish_processed_; }

void UnaryCallState::SetFinishProcessed() noexcept {
    UASSERT(!finish_processed_);
    finish_processed_ = true;
}

bool UnaryCallState::IsStatusExtracted() const noexcept { return status_extracted_; }

void UnaryCallState::SetStatusExtracted() noexcept {
    UASSERT(!status_extracted_);
    status_extracted_ = true;
}

void UnaryCallState::EmplaceFinishAsyncMethodInvocation() {
    UASSERT(!invocation_.has_value());
    invocation_.emplace();
}

FinishAsyncMethodInvocation& UnaryCallState::GetFinishAsyncMethodInvocation() noexcept {
    UASSERT(invocation_.has_value());
    return *invocation_;
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

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
