#include "statistics_holder.hpp"

#include <storages/redis/impl/cluster_topology.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

void StatisticsHolder::GetStatistics(SentinelStatistics& stats, const ClusterTopology& topology) const {
    if (!storage_) {
        return;
    }

    stats.shard_group_total.Fill(*storage_);

    for (size_t i = 0; i < topology.GetShardsCount(); ++i) {
        const auto& shard = topology.GetClusterShardByIndex(i);
        shard.GetConnStats(true, stats.masters_conn_stats);
        shard.GetConnStats(false, stats.replicas_conn_stats);
        shard.GetCommandsCounter(true, stats.master_commands);
        shard.GetCommandsCounter(false, stats.slaves_commands);
    }
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
