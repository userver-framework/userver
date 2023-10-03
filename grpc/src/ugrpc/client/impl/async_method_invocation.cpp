#include <userver/ugrpc/client/impl/async_method_invocation.hpp>

#include <ugrpc/impl/status.hpp>
#include <userver/ugrpc/client/impl/async_methods.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

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

FinishAsyncMethodInvocation::FinishAsyncMethodInvocation(RpcData& rpc_data)
    : rpc_data_(rpc_data), status_(rpc_data.GetStatus()) {}

FinishAsyncMethodInvocation::~FinishAsyncMethodInvocation() { WaitWhileBusy(); }

void FinishAsyncMethodInvocation::Notify(bool ok) noexcept {
  if (ok) {
    try {
      rpc_data_.GetStatsScope().OnExplicitFinish(status_.error_code());

      if (status_.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED &&
          rpc_data_.IsDeadlinePropagated()) {
        rpc_data_.GetStatsScope().OnCancelledByDeadlinePropagation();
      }

      rpc_data_.GetStatsScope().Flush();

      parsed_gstatus_ = ParsedGStatus::ProcessStatus(status_);
    } catch (const std::exception& e) {
      LOG_LIMITED_ERROR() << "Error in FinishAsyncMethodInvocation::Notify: "
                          << e;
    }
  }
  AsyncMethodInvocation::Notify(ok);
}

ParsedGStatus& FinishAsyncMethodInvocation::GetParsedGStatus() {
  return parsed_gstatus_;
}

grpc::Status& FinishAsyncMethodInvocation::GetStatus() { return status_; }

ugrpc::impl::AsyncMethodInvocation::WaitStatus Wait(
    ugrpc::impl::AsyncMethodInvocation& invocation,
    grpc::ClientContext& context) noexcept {
  const auto status = invocation.Wait();
  if (status == ugrpc::impl::AsyncMethodInvocation::WaitStatus::kCancelled)
    context.TryCancel();

  return status;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
