#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <fmt/format.h>

#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

#include <ugrpc/impl/rpc_metadata_keys.hpp>
#include <ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/impl/status_codes.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

void SetupSpan(std::optional<tracing::InPlaceSpan>& span_holder,
               grpc::ClientContext& context, std::string_view call_name) {
  UASSERT(!span_holder);
  span_holder.emplace(utils::StrCat("grpc/", call_name));
  auto& span = span_holder->Get();

  span.DetachFromCoroStack();

  context.AddMetadata(ugrpc::impl::kXYaTraceId,
                      ugrpc::impl::ToGrpcString(span.GetTraceId()));
  context.AddMetadata(ugrpc::impl::kXYaSpanId,
                      ugrpc::impl::ToGrpcString(span.GetSpanId()));
  context.AddMetadata(ugrpc::impl::kXYaRequestId,
                      ugrpc::impl::ToGrpcString(span.GetLink()));
}

void SetErrorForSpan(RpcData& data, std::string&& message) {
  data.GetSpan().AddTag(tracing::kErrorFlag, true);
  data.GetSpan().AddTag(tracing::kErrorMessage, std::move(message));
  data.ResetSpan();
}

}  // namespace

RpcData::RpcData(std::unique_ptr<grpc::ClientContext>&& context,
                 std::string_view call_name,
                 ugrpc::impl::MethodStatistics& statistics)
    : context_(std::move(context)),
      call_name_(call_name),
      stats_scope_(statistics) {
  UASSERT(context_);
  SetupSpan(span_, *context_, call_name_);
}

RpcData::~RpcData() {
  UASSERT(!finish_.has_value());
  if (context_ && state_ != impl::State::kFinished) {
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

std::string_view RpcData::GetCallName() const noexcept {
  UASSERT(context_);
  return call_name_;
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

State RpcData::GetState() const noexcept {
  UASSERT(context_);
  return state_;
}

void RpcData::SetState(State new_state) noexcept {
  UASSERT(context_);
  state_ = new_state;
}

void RpcData::EmplaceAsyncMethodInvocation() {
  UINVARIANT(!finish_.has_value(), "'FinishAsync' already was called");
  finish_.emplace();
}

AsyncMethodInvocation& RpcData::GetAsyncMethodInvocation() noexcept {
  UASSERT(finish_.has_value());
  return finish_.value();
}

grpc::Status& RpcData::GetStatus() noexcept { return status_; }

RpcData::AsyncMethodInvocationGuard::AsyncMethodInvocationGuard(
    RpcData& data) noexcept
    : data_(data) {}

RpcData::AsyncMethodInvocationGuard::~AsyncMethodInvocationGuard() noexcept {
  UASSERT(data_.finish_.has_value());
  data_.finish_.reset();
  data_.GetStatus() = grpc::Status{};
}

void CheckOk(RpcData& data, bool ok, std::string_view stage) {
  if (!ok) {
    data.SetState(State::kFinished);
    data.GetStatsScope().OnNetworkError();
    SetErrorForSpan(data, fmt::format("Network error at '{}'", stage));
    throw RpcInterruptedError(data.GetCallName(), stage);
  }
}

void PrepareFinish(RpcData& data) {
  UINVARIANT(data.GetState() != impl::State::kFinished,
             "'Finish' called on a finished call");
  data.SetState(State::kFinished);
}

void ProcessFinishResult(RpcData& data, bool ok, grpc::Status& status) {
  UASSERT_MSG(ok,
              "ok=false in async Finish method invocation is prohibited "
              "by gRPC docs, see grpc::CompletionQueue::Next");
  data.GetStatsScope().OnExplicitFinish(status.error_code());
  if (!status.ok()) {
    data.SetState(State::kFinished);
    SetErrorForSpan(data,
                    std::string{ugrpc::impl::ToString(status.error_code())});
    return;
  }
  data.ResetSpan();
}

void PrepareRead(RpcData& data) {
  UINVARIANT(data.GetState() != impl::State::kFinished,
             "'Read' called on a finished call");
}

void PrepareWrite(RpcData& data) {
  UINVARIANT(data.GetState() == impl::State::kOpen,
             "'Write' called on a stream that is closed for writes");
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
