#include <userver/ugrpc/impl/statistics_scope.hpp>

#include <algorithm>  // for std::max

#include <userver/ugrpc/impl/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl {

RpcStatisticsScope::RpcStatisticsScope(MethodStatistics& statistics)
    : statistics_(statistics), start_time_(std::chrono::steady_clock::now()) {
    statistics_->AccountStarted();
}

RpcStatisticsScope::~RpcStatisticsScope() { Flush(); }

void RpcStatisticsScope::OnExplicitFinish(grpc::StatusCode code) noexcept {
    finish_kind_ = std::max(finish_kind_, FinishKind::kExplicit);
    finish_code_ = code;
}

void RpcStatisticsScope::OnNetworkError() noexcept { finish_kind_ = std::max(finish_kind_, FinishKind::kNetworkError); }

void RpcStatisticsScope::OnCancelledByDeadlinePropagation() noexcept {
    finish_kind_ = std::max(finish_kind_, FinishKind::kDeadlinePropagation);
}

void RpcStatisticsScope::OnDeadlinePropagated() noexcept { is_deadline_propagated_ = true; }

void RpcStatisticsScope::OnCancelled() noexcept { finish_kind_ = std::max(finish_kind_, FinishKind::kCancelled); }

void RpcStatisticsScope::SetFinishTime(std::chrono::steady_clock::time_point finish_time) noexcept {
    finish_time_ = finish_time;
}

void RpcStatisticsScope::Flush() noexcept {
    if (!start_time_) {
        return;
    }

    if (is_deadline_propagated_) {
        statistics_->AccountDeadlinePropagated();
    }

    AccountTiming();
    switch (finish_kind_) {
        case FinishKind::kAutomatic:
            statistics_->AccountInternalError();
            return;
        case FinishKind::kExplicit:
            statistics_->AccountStatus(finish_code_);
            return;
        case FinishKind::kNetworkError:
            statistics_->AccountNetworkError();
            return;
        case FinishKind::kDeadlinePropagation:
            statistics_->AccountCancelledByDeadlinePropagation();
            return;
        case FinishKind::kCancelled:
            statistics_->AccountCancelled();
            return;
    }
    UASSERT_MSG(false, "Invalid FinishKind");
}

void RpcStatisticsScope::RedirectTo(MethodStatistics& statistics) {
    if (!start_time_) return;

    // Relies on the fact that all metrics, except for 'started' metric,
    // are only actually accounted in Flush.
    statistics_->MoveStartedTo(statistics);
    statistics_ = statistics;
}

void RpcStatisticsScope::AccountTiming() noexcept {
    if (!start_time_) return;

    const auto finish_time = finish_time_.has_value() ? *finish_time_ : std::chrono::steady_clock::now();
    statistics_->AccountTiming(std::chrono::duration_cast<std::chrono::milliseconds>(finish_time - *start_time_));
    start_time_.reset();
}

}  // namespace ugrpc::impl

USERVER_NAMESPACE_END
