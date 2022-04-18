#include "pool_statistics.hpp"

#include <userver/formats/json/value_builder.hpp>

#include <userver/utils/statistics/percentile_format_json.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::stats {

namespace {

formats::json::Value PoolConnectionStatisticsToJson(
    const PoolConnectionStatistics& stats) {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};
  builder["closed"] = stats.closed.Load();
  builder["created"] = stats.created.Load();
  builder["overload"] = stats.overload.Load();
  builder["active"] = stats.active.Load();
  builder["busy"] = stats.busy.Load();

  return builder.ExtractValue();
}

formats::json::Value PoolQueryStatisticsToJson(
    const PoolQueryStatistics& stats) {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};
  builder["total"] = stats.total.Load();
  builder["error"] = stats.error.Load();
  builder["timings"] = USERVER_NAMESPACE::utils::statistics::PercentileToJson(
      stats.timings.GetStatsForPeriod());

  return builder.ExtractValue();
}

}  // namespace

formats::json::Value PoolStatisticsToJson(const PoolStatistics& stats) {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};
  builder["connections"] = PoolConnectionStatisticsToJson(stats.connections);
  builder["queries"] = PoolQueryStatisticsToJson(stats.queries);
  builder["inserts"] = PoolQueryStatisticsToJson(stats.inserts);

  return builder.ExtractValue();
}

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
