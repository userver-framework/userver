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
  writer["total"] = stats.total;
  writer["error"] = stats.error;
  writer["timings"] = stats.timings;
}

void DumpMetric(USERVER_NAMESPACE::utils::statistics::Writer& writer,
                const PoolConnectionStatistics& stats) {
  writer["closed"] = stats.closed;
  writer["created"] = stats.created;
  writer["overload"] = stats.overload;
  writer["active"] = stats.active;
  writer["busy"] = stats.busy;
}

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
