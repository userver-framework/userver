#include <storages/mongo/congestion_control/sensor.hpp>

#include <algorithm>  // for std::max

#include <storages/mongo/pool_impl.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::cc {

namespace {

AccumulatedData SumStats(const stats::PoolStatistics& stats) {
  AccumulatedData result;
  for (const auto& [coll, coll_stats] : stats.collections) {
    for (const auto& [op, op_stats] : coll_stats->items) {
      result.total_queries += op_stats->GetTotalQueries().value;
      result.timeouts +=
          op_stats->GetCounter(stats::ErrorType::kNetwork).value +
          op_stats->GetCounter(stats::ErrorType::kClusterUnavailable).value;
      result.timings_sum += op_stats->timings_sum.Load().value;
    }
  }
  return result;
}

}  // namespace

AccumulatedData operator-(const AccumulatedData& lhs,
                          const AccumulatedData& rhs) noexcept {
  AccumulatedData result;
  result.total_queries = lhs.total_queries - rhs.total_queries;
  result.timeouts = lhs.timeouts - rhs.timeouts;
  result.timings_sum = lhs.timings_sum - rhs.timings_sum;
  return result;
}

Sensor::Sensor(impl::PoolImpl& pool) : pool_(pool) {}

Sensor::Data Sensor::GetCurrent() {
  const auto& stats = pool_.GetStatistics();
  const auto new_data = SumStats(stats);
  const auto last_data = std::exchange(last_data_, new_data);

  const auto diff = new_data - last_data;
  const auto total = std::max(diff.total_queries, std::uint64_t{1});

  const auto timings_sum_rate = diff.timings_sum / total;
  LOG_TRACE() << "timings avg = " << timings_sum_rate << "ms";

  const auto timeout_rate = static_cast<double>(diff.timeouts) / total;
  LOG_TRACE() << "timeout rate = " << timeout_rate;

  const auto current_load = pool_.SizeApprox();
  return {total, diff.timeouts, timings_sum_rate, current_load};
}

}  // namespace storages::mongo::cc

USERVER_NAMESPACE_END
