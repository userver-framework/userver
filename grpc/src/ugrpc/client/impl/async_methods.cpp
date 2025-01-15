#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <fmt/format.h>
#include <grpcpp/support/status.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/source_location.hpp>

#include <ugrpc/client/impl/client_configs.hpp>
#include <ugrpc/impl/rpc_metadata.hpp>
#include <ugrpc/impl/status.hpp>
#include <userver/tracing/opentelemetry.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/impl/deadline_timepoint.hpp>
#include <userver/ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {
constexpr std::string_view kDefaultOtelTraceFlags = "01";

void SetupSpan(
    std::optional<tracing::InPlaceSpan>& span_holder,
    grpc::ClientContext& context,
    std::string_view call_name
) {
    UASSERT(!span_holder);
    span_holder.emplace(utils::StrCat("external_grpc/", call_name), utils::impl::SourceLocation::Current());
    auto& span = span_holder->Get();

    span.DetachFromCoroStack();

    context.AddMetadata(ugrpc::impl::kXYaTraceId, ugrpc::impl::ToGrpcString(span.GetTraceId()));
    context.AddMetadata(ugrpc::impl::kXYaSpanId, ugrpc::impl::ToGrpcString(span.GetSpanId()));
    context.AddMetadata(ugrpc::impl::kXYaRequestId, ugrpc::impl::ToGrpcString(span.GetLink()));

    auto traceparent =
        tracing::opentelemetry::BuildTraceParentHeader(span.GetTraceId(), span.GetSpanId(), kDefaultOtelTraceFlags);

    if (!traceparent.has_value()) {
        LOG_LIMITED_DEBUG() << fmt::format("Cannot build opentelemetry traceparent header ({})", traceparent.error());
        return;
    }
    context.AddMetadata(ugrpc::impl::kTraceParent, ugrpc::impl::ToGrpcString(traceparent.value()));
}

void SetErrorForSpan(tracing::Span& span, const std::string& error_message) {
    span.SetLogLevel(logging::Level::kWarning);
    span.AddTag(tracing::kErrorFlag, true);
    span.AddTag(tracing::kErrorMessage, error_message);
}

void SetErrorAndResetSpan(RpcData& data, const std::string& error_message) {
    SetErrorForSpan(data.GetSpan(), error_message);
    data.ResetSpan();
}

void SetStatusDetailsForSpan(
    tracing::Span& span,
    const grpc::Status& status,
    const std::optional<std::string>& error_details
) {
    span.AddTag("grpc_code", std::string{ugrpc::ToString(status.error_code())});
    if (!status.ok()) {
        SetErrorForSpan(span, error_details.value_or(status.error_message()));
    }
}

}  // namespace

RpcConfigValues::RpcConfigValues(const dynamic_config::Snapshot& config)
    : enforce_task_deadline(config[kEnforceClientTaskDeadline]) {}

ParsedGStatus ParsedGStatus::ProcessStatus(const grpc::Status& status) {
    if (status.ok()) {
        return {};
    }
    auto gstatus = ugrpc::impl::ToGoogleRpcStatus(status);
    std::optional<std::string> gstatus_string;
    if (gstatus) {
        gstatus_string = ugrpc::impl::GetGStatusLimitedMessage(*gstatus);
    }

    return ParsedGStatus{std::move(gstatus), std::move(gstatus_string)};
}

RpcData::RpcData(impl::CallParams&& params, CallKind call_kind)
    : context_(std::move(params.context)),
      client_name_(params.client_name),
      call_name_(std::move(params.call_name)),
      stats_scope_(params.statistics),
      queue_(params.queue),
      config_values_(params.config),
      mws_(params.mws),
      call_kind_(call_kind) {
    UASSERT(context_);
    UASSERT(!client_name_.empty());
    SetupSpan(span_, *context_, call_name_.Get());
}

RpcData::~RpcData() {
    invocation_.emplace<std::monostate>();

    if (context_ && !IsFinished()) {
        UASSERT(span_);
        SetErrorAndResetSpan(*this, "Abandoned");
        context_->TryCancel();
    }
}

const grpc::ClientContext& RpcData::GetContext() const noexcept {
    UASSERT(context_);
    return *context_;
}

grpc::ClientContext& RpcData::GetContext() noexcept {
    UASSERT(context_);
    return *context_;
}

grpc::CompletionQueue& RpcData::GetQueue() const noexcept {
    UASSERT(context_);
    return queue_;
}

const RpcConfigValues& RpcData::GetConfigValues() const noexcept {
    UASSERT(context_);
    return config_values_;
}

const Middlewares& RpcData::GetMiddlewares() const noexcept {
    UASSERT(context_);
    return mws_;
}

std::string_view RpcData::GetCallName() const noexcept {
    UASSERT(context_);
    return call_name_.Get();
}

std::string_view RpcData::GetClientName() const noexcept {
    UASSERT(context_);
    return client_name_;
}

tracing::Span& RpcData::GetSpan() noexcept {
    UASSERT(context_);
    UASSERT(span_);
    return span_->Get();
}

CallKind RpcData::GetCallKind() const noexcept { return call_kind_; }

void RpcData::ResetSpan() noexcept {
    UASSERT(context_);
    UASSERT(span_);
    span_.reset();
}

ugrpc::impl::RpcStatisticsScope& RpcData::GetStatsScope() noexcept {
    UASSERT(context_);
    return stats_scope_;
}

void RpcData::SetFinished() noexcept {
    UASSERT(context_);
    UINVARIANT(!is_finished_, "Tried to finish already finished call");
    is_finished_ = true;
}

bool RpcData::IsFinished() const noexcept {
    UASSERT(context_);
    return is_finished_;
}

void RpcData::SetDeadlinePropagated() noexcept {
    UASSERT(context_);
    stats_scope_.OnDeadlinePropagated();
    is_deadline_propagated_ = true;
}

bool RpcData::IsDeadlinePropagated() const noexcept {
    UASSERT(context_);
    return is_deadline_propagated_;
}

void RpcData::SetWritesFinished() noexcept {
    UASSERT(context_);
    UASSERT(!writes_finished_);
    writes_finished_ = true;
}

bool RpcData::AreWritesFinished() const noexcept {
    UASSERT(context_);
    return writes_finished_;
}

void RpcData::EmplaceAsyncMethodInvocation() {
    UINVARIANT(
        std::holds_alternative<std::monostate>(invocation_),
        "Another method is already running for this RPC concurrently"
    );
    invocation_.emplace<AsyncMethodInvocation>();
}

void RpcData::EmplaceFinishAsyncMethodInvocation() {
    UINVARIANT(
        std::holds_alternative<std::monostate>(invocation_),
        "Another method is already running for this RPC concurrently"
    );
    invocation_.emplace<FinishAsyncMethodInvocation>(*this);
}

AsyncMethodInvocation& RpcData::GetAsyncMethodInvocation() noexcept {
    UASSERT(std::holds_alternative<AsyncMethodInvocation>(invocation_));
    return std::get<AsyncMethodInvocation>(invocation_);
}

FinishAsyncMethodInvocation& RpcData::GetFinishAsyncMethodInvocation() noexcept {
    UASSERT(std::holds_alternative<FinishAsyncMethodInvocation>(invocation_));
    return std::get<FinishAsyncMethodInvocation>(invocation_);
}

bool RpcData::HoldsAsyncMethodInvocationDebug() noexcept {
    return std::holds_alternative<AsyncMethodInvocation>(invocation_);
}

bool RpcData::HoldsFinishAsyncMethodInvocationDebug() noexcept {
    return std::holds_alternative<FinishAsyncMethodInvocation>(invocation_);
}

bool RpcData::IsFinishProcessed() const noexcept { return finish_processed_; }

void RpcData::SetFinishProcessed() noexcept {
    UASSERT(!finish_processed_);
    finish_processed_ = true;
}

bool RpcData::IsStatusExtracted() const noexcept { return status_extracted_; }

void RpcData::SetStatusExtracted() noexcept {
    UASSERT(!status_extracted_);
    status_extracted_ = true;
}

grpc::Status& RpcData::GetStatus() noexcept { return status_; }

ParsedGStatus& RpcData::GetParsedGStatus() noexcept { return parsed_g_status_; }

RpcData::AsyncMethodInvocationGuard::AsyncMethodInvocationGuard(RpcData& data) noexcept : data_(data) {
    UASSERT(!std::holds_alternative<std::monostate>(data_.invocation_));
}

RpcData::AsyncMethodInvocationGuard::~AsyncMethodInvocationGuard() noexcept {
    UASSERT(!std::holds_alternative<std::monostate>(data_.invocation_));
    if (!disarm_) {
        data_.invocation_.emplace<std::monostate>();
    }
}

void CheckOk(RpcData& data, AsyncMethodInvocation::WaitStatus status, std::string_view stage) {
    if (status == impl::AsyncMethodInvocation::WaitStatus::kError) {
        data.SetFinished();
        data.GetStatsScope().OnNetworkError();
        data.GetStatsScope().Flush();
        SetErrorAndResetSpan(data, fmt::format("Network error at '{}'", stage));
        throw RpcInterruptedError(data.GetCallName(), stage);
    } else if (status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        data.SetFinished();
        data.GetStatsScope().OnCancelled();
        SetErrorAndResetSpan(data, fmt::format("Network error at '{}' (task cancelled)", stage));
        throw RpcCancelledError(data.GetCallName(), stage);
    }
}

void PrepareFinish(RpcData& data) {
    UINVARIANT(!data.IsFinished(), "'Finish' called on a finished call");
    data.SetFinished();
}

void ProcessFinish(RpcData& data, utils::function_ref<void(RpcData& data, const grpc::Status& status)> post_finish) {
    const auto& status = data.GetStatus();

    data.GetStatsScope().OnExplicitFinish(status.error_code());
    data.GetStatsScope().Flush();

    post_finish(data, status);

    auto& parsed_gstatus = data.GetParsedGStatus();
    parsed_gstatus = ParsedGStatus::ProcessStatus(status);

    SetStatusDetailsForSpan(data.GetSpan(), status, parsed_gstatus.gstatus_string);
    data.ResetSpan();
}

void CheckFinishStatus(RpcData& data) {
    auto& status = data.GetStatus();
    if (!status.ok()) {
        auto& parsed_gstatus = data.GetParsedGStatus();
        impl::ThrowErrorWithStatus(
            data.GetCallName(),
            std::move(status),
            std::move(parsed_gstatus.gstatus),
            std::move(parsed_gstatus.gstatus_string)
        );
    }
}

void ProcessFinishResult(
    RpcData& data,
    AsyncMethodInvocation::WaitStatus wait_status,
    utils::function_ref<void(RpcData& data, const grpc::Status& status)> post_finish,
    bool throw_on_error
) {
    const auto ok = wait_status == impl::AsyncMethodInvocation::WaitStatus::kOk;
    UASSERT_MSG(
        ok,
        "ok=false in async Finish method invocation is prohibited "
        "by gRPC docs, see grpc::CompletionQueue::Next"
    );

    ProcessFinish(data, post_finish);

    if (throw_on_error) {
        CheckFinishStatus(data);
    }
}

void PrepareRead(RpcData& data) { UINVARIANT(!data.IsFinished(), "'Read' called on a finished call"); }

void PrepareWrite(RpcData& data) {
    UINVARIANT(!data.AreWritesFinished(), "'Write' called on a stream that is closed for writes");
}

void PrepareWriteAndCheck(RpcData& data) {
    UINVARIANT(!data.AreWritesFinished(), "'WriteAndCheck' called on a stream that is closed for writes");
    UINVARIANT(!data.IsFinished(), "'WriteAndCheck' called on a finished call");
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
