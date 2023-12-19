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
  // If the task is cancelled, then this is what typically happens:
  //
  // 1. after cancelling the wait, the task calls OnCancelled
  // 2. the task calls ClientContext::TryCancel
  // 3. grpc-core thread notifies CompletionQueue
  // 4. FinishAsyncMethodInvocation::Notify calls Flush, which accounts
  //    is_cancelled_
  // 5. Notify wakes up the task
  //
  // However, there is a small chance the request completes on its own
  // between (1) and (2). It will call Notify and Flush, which might miss
  // is_cancelled_ from the task, which is being cancelled in parallel, and just
  // write normal request completion stats. This is fine, because the actual
  // request was not cancelled because of the task cancellation.
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
