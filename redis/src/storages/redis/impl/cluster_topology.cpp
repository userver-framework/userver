#include "cluster_topology.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

ClusterTopology::ClusterTopology(
    size_t version, std::chrono::steady_clock::time_point timestamp,
    ClusterShardHostInfos infos, Password password,
    const std::shared_ptr<engine::ev::ThreadPool>& /*redis_thread_pool*/,
    NodesStorage& nodes)
    : infos_(std::move(infos)),
      password_(std::move(password)),
      version_(version),
      timestamp_(timestamp) {
  {
    auto HostPortToString = [](const std::pair<std::string, int> host_port) {
      return host_port.first + ":" + std::to_string(host_port.second);
    };
    cluster_shards_.reserve(infos_.size());
    size_t shard_index = 0;
    for (const auto& info : infos_) {
      const auto& master_host_port = HostPortToString(info.master.HostPort());
      /// TODO: nodes from CLUSTER SLOTS must be present in CLUSTER NODES
      /// Do we need additional check or exception from 'at' will be enough?
      auto master = nodes.Get(master_host_port);
      std::vector<ClusterShard::RedisPtr> replicas;
      replicas.reserve(info.slaves.size());
      for (const auto& replica : info.slaves) {
        replicas.push_back(nodes.Get(HostPortToString(replica.HostPort())));
      }
      cluster_shards_.emplace_back(shard_index++, std::move(master),
                                   std::move(replicas));
    }
  }

  {
    size_t shard_index = 0;
    for (const auto& info : infos_) {
      for (const auto& interval : info.slot_intervals) {
        for (size_t i = interval.slot_min; i <= interval.slot_max; ++i) {
          slot_to_shard_[i] = shard_index;
        }
      }
      ++shard_index;
    }
  }
}

ClusterTopology::~ClusterTopology() = default;

bool ClusterTopology::IsReady(WaitConnectedMode mode) const {
  return !cluster_shards_.empty() &&
         std::all_of(
             cluster_shards_.begin(), cluster_shards_.end(),
             [mode](const ClusterShard& shard) { return shard.IsReady(mode); });
}

bool ClusterTopology::HasSameInfos(const ClusterShardHostInfos& infos) const {
  /// other fields calculated from infos_
  if (infos_.size() != infos.size()) {
    return false;
  }
  for (size_t i = 0; i < infos.size(); ++i) {
    const auto& l = infos_[i];
    const auto& r = infos[i];
    if (l.master.HostPort() != r.master.HostPort()) return false;
    if (l.slaves.size() != r.slaves.size()) return false;
    for (size_t j = 0; j < l.slaves.size(); ++j) {
      const auto& lslave = l.slaves[j];
      const auto& rslave = r.slaves[j];
      if (lslave.HostPort() != rslave.HostPort()) return false;
    }
    if (l.slot_intervals != r.slot_intervals) {
      return false;
    }
  }
  return true;
}

void ClusterTopology::GetStatistics(
    const MetricsSettings& settings, SentinelStatistics& stats,
    const std::vector<std::string>& shard_names) const {
  size_t shard_index = 0;
  for (const auto& shard : cluster_shards_) {
    const auto& shard_name = shard_names.at(shard_index++);
    auto master_stats = shard.GetStatistics(true, settings);
    auto replica_stats = shard.GetStatistics(false, settings);
    stats.shard_group_total.Add(master_stats.shard_total);
    stats.shard_group_total.Add(replica_stats.shard_total);
    stats.masters.emplace(shard_name, std::move(master_stats));
    stats.slaves.emplace(shard_name, std::move(replica_stats));
  }
}
}  // namespace redis

USERVER_NAMESPACE_END
