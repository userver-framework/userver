#include <storages/postgres/congestion_control/sensor.hpp>

#include <storages/postgres/detail/pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::cc {

Sensor::Sensor(detail::ConnectionPool& pool) : pool_(pool) {}

Sensor::Data Sensor::GetCurrent() {
  const auto& stats = pool_.GetStatistics();
  auto new_timeouts =
      stats.transaction.execute_timeout + stats.connection.error_timeout;
  auto diff_timeouts = new_timeouts - last_timeouted_queries;
  last_timeouted_queries = new_timeouts;

  auto new_total = stats.transaction.total;
  auto diff_total = new_total - last_total_queries;
  last_total_queries = new_total;
  if (diff_total == 0) diff_total = 1;

  auto timeout_rate = static_cast<double>(diff_timeouts) / diff_total;
  LOG_DEBUG() << "timeout rate = " << timeout_rate;

  auto current_limit = stats.connection.maximum;
  return {diff_total, diff_timeouts, current_limit};
}

}  // namespace storages::postgres::cc

USERVER_NAMESPACE_END
