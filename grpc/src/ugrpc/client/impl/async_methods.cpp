#include <userver/ugrpc/client/impl/async_methods.hpp>

#include <fmt/format.h>

#include <userver/tracing/tags.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>

#include <ugrpc/impl/rpc_metadata_keys.hpp>
#include <ugrpc/impl/to_string.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
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
      state_(State::kOpen),
      remote_data_(std::make_unique<RemoteData>(statistics)) {
  UASSERT(context_);
  SetupSpan(remote_data_->span, *context_, call_name_);
}

RpcData::~RpcData() {
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
  UASSERT(remote_data_->span);
  return remote_data_->span->Get();
}

void RpcData::ResetSpan() noexcept {
  UASSERT(context_);
  UASSERT(remote_data_->span);
  remote_data_->span.reset();
}

ugrpc::impl::RpcStatisticsScope& RpcData::GetStatsScope() noexcept {
  UASSERT(context_);
  return remote_data_->stats_scope;
}

State RpcData::GetState() const noexcept {
  UASSERT(context_);
  return state_;
}

void RpcData::SetState(State new_state) noexcept {
  UASSERT(context_);
  state_ = new_state;
}

RpcData::RemoteData::RemoteData(ugrpc::impl::MethodStatistics& statistics)
    : stats_scope(statistics) {}

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

void ProcessFinishResult(RpcData& data, bool ok, grpc::Status&& status) {
  UASSERT_MSG(ok,
              "ok=false in async Finish method invocation is prohibited "
              "by gRPC docs, see grpc::CompletionQueue::Next");
  data.GetStatsScope().OnExplicitFinish(status.error_code());
  if (!status.ok()) {
    data.SetState(State::kFinished);
    SetErrorForSpan(data,
                    std::string{ugrpc::impl::ToString(status.error_code())});
    impl::ThrowErrorWithStatus(data.GetCallName(), std::move(status));
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
