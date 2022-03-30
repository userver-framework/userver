#include "pool_statistics.hpp"

#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::stats {

formats::json::Value PoolStatisticsToJson(const PoolStatistics& stats) {
  formats::json::ValueBuilder builder{formats::json::Type::kObject};
  builder["closed"] = stats.closed.Load();
  builder["created"] = stats.created.Load();
  builder["overload"] = stats.overload.Load();

  return builder.ExtractValue();
}

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
