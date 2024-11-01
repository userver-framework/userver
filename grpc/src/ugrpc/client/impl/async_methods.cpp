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

void SetStatusDetailsForSpan(RpcData& data, grpc::Status& status, const std::optional<std::string>& message) {
    data.GetSpan().SetLogLevel(logging::Level::kWarning);
    data.GetSpan().AddTag(tracing::kErrorFlag, true);
    data.GetSpan().AddTag("grpc_code", std::string{ugrpc::ToString(status.error_code())});
    if (message.has_value()) {
        data.GetSpan().AddTag(tracing::kErrorMessage, *message);
    } else {
        data.GetSpan().AddTag(tracing::kErrorMessage, status.error_message());
    }
    data.ResetSpan();
}

void SetErrorForSpan(RpcData& data, std::string&& message) {
    data.GetSpan().SetLogLevel(logging::Level::kWarning);
    data.GetSpan().AddTag(tracing::kErrorFlag, true);
    data.GetSpan().AddTag(tracing::kErrorMessage, std::move(message));
    data.ResetSpan();
}

}  // namespace

RpcConfigValues::RpcConfigValues(const dynamic_config::Snapshot& config)
    : enforce_task_deadline(config[kEnforceClientTaskDeadline]) {}

FutureImpl::FutureImpl(RpcData& data) noexcept : data_(&data) {}

FutureImpl::FutureImpl(FutureImpl&& other) noexcept : data_(std::exchange(other.data_, nullptr)) {}

FutureImpl& FutureImpl::operator=(FutureImpl&& other) noexcept {
    if (this == &other) return *this;
    data_ = std::exchange(other.data_, nullptr);
    return *this;
}

bool FutureImpl::IsReady() const noexcept {
    UINVARIANT(data_, "IsReady should be called only before 'Get'");
    auto& method = data_->GetAsyncMethodInvocation();
    return method.IsReady();
}

RpcData* FutureImpl::GetData() noexcept { return data_; }

void FutureImpl::ClearData() noexcept { data_ = nullptr; }

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
    UASSERT(std::holds_alternative<std::monostate>(invocation_));

    if (context_ && !IsFinished()) {
        UASSERT(span_);
        SetErrorForSpan(*this, "Abandoned");
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

grpc::Status& RpcData::GetStatus() noexcept { return status_; }

RpcData::AsyncMethodInvocationGuard::AsyncMethodInvocationGuard(RpcData& data) noexcept : data_(data) {}

RpcData::AsyncMethodInvocationGuard::~AsyncMethodInvocationGuard() noexcept {
    UASSERT(!std::holds_alternative<std::monostate>(data_.invocation_));
    if (!disarm_) {
        data_.invocation_.emplace<std::monostate>();

        data_.GetStatus() = grpc::Status{};
    }
}

void CheckOk(RpcData& data, AsyncMethodInvocation::WaitStatus status, std::string_view stage) {
    if (status == impl::AsyncMethodInvocation::WaitStatus::kError) {
        data.SetFinished();
        data.GetStatsScope().OnNetworkError();
        data.GetStatsScope().Flush();
        SetErrorForSpan(data, fmt::format("Network error at '{}'", stage));
        throw RpcInterruptedError(data.GetCallName(), stage);
    } else if (status == impl::AsyncMethodInvocation::WaitStatus::kCancelled) {
        data.SetFinished();
        data.GetStatsScope().OnCancelled();
        SetErrorForSpan(data, fmt::format("Network error at '{}' (task cancelled)", stage));
        throw RpcCancelledError(data.GetCallName(), stage);
    }
}

void PrepareFinish(RpcData& data) {
    UINVARIANT(!data.IsFinished(), "'Finish' called on a finished call");
    data.SetFinished();
}

void ProcessFinishResult(
    RpcData& data,
    AsyncMethodInvocation::WaitStatus wait_status,
    grpc::Status&& status,
    ParsedGStatus&& parsed_gstatus,
    utils::function_ref<void(RpcData& data, const grpc::Status& status)> post_finish,
    bool throw_on_error
) {
    const auto ok = wait_status == impl::AsyncMethodInvocation::WaitStatus::kOk;
    UASSERT_MSG(
        ok,
        "ok=false in async Finish method invocation is prohibited "
        "by gRPC docs, see grpc::CompletionQueue::Next"
    );
    data.GetStatsScope().OnExplicitFinish(status.error_code());
    data.GetStatsScope().Flush();

    post_finish(data, status);

    if (!status.ok()) {
        SetStatusDetailsForSpan(data, status, parsed_gstatus.gstatus_string);

        if (throw_on_error) {
            impl::ThrowErrorWithStatus(
                data.GetCallName(),
                std::move(status),
                std::move(parsed_gstatus.gstatus),
                std::move(parsed_gstatus.gstatus_string)
            );
        }
    } else {
        data.GetSpan().AddTag("grpc_code", std::string{ugrpc::ToString(grpc::StatusCode::OK)});
        data.ResetSpan();
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
