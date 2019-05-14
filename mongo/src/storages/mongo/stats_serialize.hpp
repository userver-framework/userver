#pragma once

#include <formats/json/value_builder.hpp>
#include <storages/mongo/stats.hpp>

namespace storages::mongo::stats {

void PoolStatisticsToJson(const PoolStatistics&, formats::json::ValueBuilder&);

}  // namespace storages::mongo::stats
