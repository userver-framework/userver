#pragma once

#include <storages/mongo/stats.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/utils/statistics/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::stats {

void DumpMetric(utils::statistics::Writer& writer,
                const OperationStatisticsItem& item);

void DumpMetric(utils::statistics::Writer& writer,
                const PoolConnectStatistics& conn_stats);

void DumpMetric(utils::statistics::Writer&, const PoolStatistics&,
                StatsVerbosity);

}  // namespace storages::mongo::stats

USERVER_NAMESPACE_END
