#pragma once

#include <userver/utils/statistics/relaxed_counter.hpp>

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse::stats {

using Counter = USERVER_NAMESPACE::utils::statistics::RelaxedCounter<uint64_t>;

struct PoolStatistics final {
  Counter overload;
  Counter closed;
  Counter created;
};

formats::json::Value PoolStatisticsToJson(const PoolStatistics& stats);

}  // namespace storages::clickhouse::stats

USERVER_NAMESPACE_END
