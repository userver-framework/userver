#include "pool_statistics.hpp"

#include <userver/formats/json/value_builder.hpp>

#include <userver/utils/statistics/percentile_format_json.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::stats {

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const PoolStatistics& stats) {
  writer["connections"] = stats.connections;
  writer["queries"] = stats.queries;
  writer["inserts"] = stats.inserts;
}

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const PoolQueryStatistics& stats) {
  writer["total"] = stats.total.Load();
  writer["error"] = stats.error.Load();
  writer["timings"] = stats.timings.GetStatsForPeriod();
}

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats) {
  writer["closed"] = stats.closed.Load();
  writer["created"] = stats.created.Load();
  writer["overload"] = stats.overload.Load();
  writer["active"] = stats.active.Load();
  writer["busy"] = stats.busy.Load();
}

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
