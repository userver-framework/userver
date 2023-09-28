#include <userver/ugrpc/impl/statistics_scope.hpp>

#include <algorithm>  // for std::max

#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

RpcStatisticsScope::RpcStatisticsScope(MethodStatistics& statistics)
    : statistics_(statistics), start_time_(std::chrono::steady_clock::now()) {
  statistics_.AccountStarted();
}

RpcStatisticsScope::~RpcStatisticsScope() { Flush(); }

void RpcStatisticsScope::OnExplicitFinish(grpc::StatusCode code) {
  finish_kind_ = std::max(finish_kind_, FinishKind::kExplicit);
  finish_code_ = code;
}

void RpcStatisticsScope::OnNetworkError() {
  finish_kind_ = std::max(finish_kind_, FinishKind::kNetworkError);
}

void RpcStatisticsScope::OnCancelledByDeadlinePropagation() {
  finish_kind_ = std::max(finish_kind_, FinishKind::kDeadlinePropagation);
}

void RpcStatisticsScope::OnDeadlinePropagated() {
  statistics_.AccountDeadlinePropagated();
}

void RpcStatisticsScope::OnCancelled() {
  is_cancelled_.store(true, std::memory_order_relaxed);
}

void RpcStatisticsScope::Flush() {
  if (!start_time_) {
    return;
  }

  if (is_cancelled_.load()) {
    finish_kind_ = std::max(finish_kind_, FinishKind::kCancelled);
  }

  AccountTiming();
  switch (finish_kind_) {
    case FinishKind::kAutomatic:
      statistics_.AccountStatus(grpc::StatusCode::UNKNOWN);
      statistics_.AccountInternalError();
      return;
    case FinishKind::kExplicit:
      statistics_.AccountStatus(finish_code_);
      return;
    case FinishKind::kNetworkError:
      statistics_.AccountNetworkError();
      return;
    case FinishKind::kDeadlinePropagation:
      statistics_.AccountCancelledByDeadlinePropagation();
      return;
    case FinishKind::kCancelled:
      statistics_.AccountCancelled();
      return;
  }
  UASSERT_MSG(false, "Invalid FinishKind");
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
