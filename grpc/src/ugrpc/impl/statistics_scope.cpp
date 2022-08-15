#include <userver/ugrpc/impl/statistics_scope.hpp>

#include <algorithm>  // for std::max

#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

RpcStatisticsScope::RpcStatisticsScope(MethodStatistics& statistics)
    : statistics_(statistics), start_time_(std::chrono::steady_clock::now()) {
  statistics_.AccountStarted();
}

RpcStatisticsScope::~RpcStatisticsScope() {
  AccountStatus();
  AccountTiming();
}

void RpcStatisticsScope::OnExplicitFinish(grpc::StatusCode code) {
  finish_kind_ = std::max(finish_kind_, FinishKind::kExplicit);
  finish_code_ = code;

  // The service might keep doing something after calling 'stream.Finish()' -
  // that time is not accounted for.
  AccountTiming();
}

void RpcStatisticsScope::OnNetworkError() {
  finish_kind_ = std::max(finish_kind_, FinishKind::kNetworkError);
}

void RpcStatisticsScope::AccountStatus() {
  switch (finish_kind_) {
    case FinishKind::kAutomatic:
      statistics_.AccountStatus(grpc::StatusCode::UNKNOWN);
      statistics_.AccountInternalError();
      break;
    case FinishKind::kExplicit:
      statistics_.AccountStatus(finish_code_);
      break;
    case FinishKind::kNetworkError:
      statistics_.AccountNetworkError();
      break;
    default:
      UASSERT_MSG(false, "Invalid FinishKind");
  }
}

void RpcStatisticsScope::AccountTiming() {
  if (!start_time_) return;

  statistics_.AccountTiming(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - *start_time_));
  start_time_.reset();
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
