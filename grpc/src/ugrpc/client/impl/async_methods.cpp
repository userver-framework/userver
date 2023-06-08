#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <fmt/format.h>
#include <grpcpp/support/status.h>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <ugrpc/client/impl/client_configs.hpp>
#include <ugrpc/impl/rpc_metadata_keys.hpp>
#include <ugrpc/impl/status.hpp>
#include <ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/impl/deadline_timepoint.hpp>
#include <userver/ugrpc/impl/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void SetupSpan(std::optional<tracing::InPlaceSpan>& span_holder,
               grpc::ClientContext& context, std::string_view call_name) {
  UASSERT(!span_holder);
  span_holder.emplace(utils::StrCat("grpc/", call_name),
                      utils::impl::SourceLocation::Current());
  auto& span = span_holder->Get();

  span.DetachFromCoroStack();

  context.AddMetadata(ugrpc::impl::kXYaTraceId,
                      ugrpc::impl::ToGrpcString(span.GetTraceId()));
  context.AddMetadata(ugrpc::impl::kXYaSpanId,
                      ugrpc::impl::ToGrpcString(span.GetSpanId()));
  context.AddMetadata(ugrpc::impl::kXYaRequestId,
                      ugrpc::impl::ToGrpcString(span.GetLink()));
}

void SetStatusDetailsForSpan(RpcData& data, grpc::Status& status,
                             const std::optional<std::string>& message) {
  data.GetSpan().AddTag(tracing::kErrorFlag, true);
  data.GetSpan().AddTag(
      "grpc_code", std::string{ugrpc::impl::ToString(status.error_code())});
  if (message.has_value()) {
    data.GetSpan().AddTag(tracing::kErrorMessage, *message);
  } else {
    data.GetSpan().AddTag(tracing::kErrorMessage, status.error_message());
  }
  data.ResetSpan();
}

void SetErrorForSpan(RpcData& data, std::string&& message) {
  data.GetSpan().AddTag(tracing::kErrorFlag, true);
  data.GetSpan().AddTag(tracing::kErrorMessage, std::move(message));
  data.ResetSpan();
}

void UpdateDeadline(RpcData& data) {
  // Disable by experiment
  if (!utils::impl::kGrpcClientDeadlinePropagationExperiment.IsEnabled()) {
    return;
  }

  // Disable by config
  if (!data.GetConfigValues().enforce_task_deadline) {
    return;
  }

  auto& span = data.GetSpan();
  auto& context = data.GetContext();

  const auto context_deadline =
      engine::Deadline::FromTimePoint(context.deadline());
  const engine::Deadline task_deadline =
      server::request::GetTaskInheritedDeadline();

  if (!task_deadline.IsReachable() && !context_deadline.IsReachable()) {
    return;
  }

  engine::Deadline result_deadline{context_deadline};

  if (task_deadline < context_deadline) {
    span.AddTag("deadline_updated", true);
    data.SetDeadlinePropagated();
    result_deadline = task_deadline;
  }

  auto result_deadline_ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          result_deadline.TimeLeft());

  context.set_deadline(result_deadline);
  span.AddTag(tracing::kTimeoutMs, result_deadline_ms.count());
}

}  // namespace

RpcConfigValues::RpcConfigValues(const dynamic_config::Snapshot& config)
    : enforce_task_deadline(config[kEnforceClientTaskDeadline]) {}

FutureImpl::FutureImpl(RpcData& data) noexcept : data_(&data) {}

FutureImpl::FutureImpl(FutureImpl&& other) noexcept
    : data_(std::exchange(other.data_, nullptr)) {}

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

RpcData::RpcData(impl::CallParams&& params)
    : context_(std::move(params.context)),
      client_name_(params.call_name),
      call_name_(params.call_name),
      stats_scope_(params.statistics),
      queue_(params.queue),
      config_values_(params.config),
      mws_(params.mws) {
  UASSERT(context_);
  UASSERT(!client_name_.empty());
  SetupSpan(span_, *context_, call_name_);
  UpdateDeadline(*this);
}

RpcData::~RpcData() {
  UASSERT(!invocation_.has_value());

  if (context_ && !IsFinished()) {
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
  return call_name_;
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
  UINVARIANT(!invocation_.has_value(),
             "Another method is already running for this RPC concurrently");
  invocation_.emplace();
}

AsyncMethodInvocation& RpcData::GetAsyncMethodInvocation() noexcept {
  UASSERT(invocation_.has_value());
  return invocation_.value();
}

grpc::Status& RpcData::GetStatus() noexcept { return status_; }

RpcData::AsyncMethodInvocationGuard::AsyncMethodInvocationGuard(
    RpcData& data) noexcept
    : data_(data) {}

RpcData::AsyncMethodInvocationGuard::~AsyncMethodInvocationGuard() noexcept {
  UASSERT(data_.invocation_.has_value());
  data_.invocation_.reset();
  data_.GetStatus() = grpc::Status{};
}

void CheckOk(RpcData& data, bool ok, std::string_view stage) {
  if (!ok) {
    data.SetFinished();
    data.GetStatsScope().OnNetworkError();
    SetErrorForSpan(data, fmt::format("Network error at '{}'", stage));
    throw RpcInterruptedError(data.GetCallName(), stage);
  }
}

void PrepareFinish(RpcData& data) {
  UINVARIANT(!data.IsFinished(), "'Finish' called on a finished call");
  data.SetFinished();
}

void ProcessFinishResult(RpcData& data, bool ok, grpc::Status& status,
                         bool throw_on_error) {
  UASSERT_MSG(ok,
              "ok=false in async Finish method invocation is prohibited "
              "by gRPC docs, see grpc::CompletionQueue::Next");
  data.GetStatsScope().OnExplicitFinish(status.error_code());

  if (!status.ok()) {
    // extract error
    std::optional<std::string> gstatus_string;

    auto gstatus = ugrpc::impl::ToGoogleRpcStatus(status);
    if (gstatus) {
      gstatus_string = ugrpc::impl::GetGStatusLimitedMessage(*gstatus);
    }

    SetStatusDetailsForSpan(data, status, gstatus_string);
    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED &&
        data.IsDeadlinePropagated()) {
      data.GetStatsScope().CancelledByDeadlinePropagation();
    }

    if (throw_on_error) {
      impl::ThrowErrorWithStatus(data.GetCallName(), std::move(status),
                                 std::move(gstatus), std::move(gstatus_string));
    }
  } else {
    data.ResetSpan();
  }
}

void PrepareRead(RpcData& data) {
  UINVARIANT(!data.IsFinished(), "'Read' called on a finished call");
}

void PrepareWrite(RpcData& data) {
  UINVARIANT(!data.AreWritesFinished(),
             "'Write' called on a stream that is closed for writes");
}

void PrepareWriteAndCheck(RpcData& data) {
  UINVARIANT(!data.AreWritesFinished(),
             "'WriteAndCheck' called on a stream that is closed for writes");
  UINVARIANT(!data.IsFinished(), "'WriteAndCheck' called on a finished call");
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
