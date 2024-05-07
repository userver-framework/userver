#include <userver/ydb/impl/stats_scope.hpp>

#include <userver/utils/datetime.hpp>

#include <ydb/impl/stats.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb::impl {

namespace {

StatsCounters& GetCountersForQuery(Stats& stats, const Query& query) {
  const auto& query_name = query.GetName();

  if (!query_name) {
    return stats.unnamed_queries;
  }

  const auto insertion_result = stats.by_query.TryEmplace(
      query_name.value().GetUnderlying(), stats.by_database_histogram_bounds);
  // No need to retain a shared_ptr. References to items are stable.
  // Items are never removed from the map.
  return *insertion_result.value;
}

StatsCounters& GetCountersForTransaction(Stats& stats,
                                         const std::string& tx_name) {
  const auto insertion_result = stats.by_transaction.TryEmplace(
      tx_name, stats.by_database_histogram_bounds);
  // No need to retain a shared_ptr. References to items are stable.
  // Items are never removed from the map.
  return *insertion_result.value;
}

}  // namespace

StatsScope::StatsScope(Stats& stats, const Query& query)
    : StatsScope(GetCountersForQuery(stats, query)) {}

StatsScope::StatsScope(TransactionTag, Stats& stats, const std::string& tx_name)
    : StatsScope(GetCountersForTransaction(stats, tx_name)) {}

StatsScope::StatsScope(StatsCounters& stats)
    : stats_(stats), start_(utils::datetime::SteadyNow()) {}

StatsScope::StatsScope(StatsScope&& other) noexcept
    : stats_(other.stats_),
      start_(other.start_),
      is_active_(std::exchange(other.is_active_, false)) {}

StatsScope::~StatsScope() {
  if (!is_active_) return;

  const auto end = utils::datetime::SteadyNow();
  const auto total_time = end - start_;
  const auto total_time_ms =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(
          total_time);

  if (is_error_) {
    ++stats_.error;
  } else {
    ++stats_.success;
  }

  stats_.timings.Account(total_time_ms.count());

  if (is_cancelled_) {
    ++stats_.cancelled;
  }
}

void StatsScope::OnError() noexcept { is_error_ = true; }

void StatsScope::OnCancelled() noexcept { is_cancelled_ = true; }

}  // namespace ydb::impl

USERVER_NAMESPACE_END
