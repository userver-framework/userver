#pragma once

#include <storages/mongo/stats.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace storages::mongo::stats {

enum class Verbosity {
  kTerse,  ///< Only pool stats and read/write overalls by collection
  kFull,
};

void PoolStatisticsToJson(const PoolStatistics&, formats::json::ValueBuilder&,
                          Verbosity);

}  // namespace storages::mongo::stats
