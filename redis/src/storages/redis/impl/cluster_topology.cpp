#include "cluster_topology.hpp"

#include <storages/redis/impl/cluster_shard.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

ClusterTopology::ClusterTopology(
    size_t version, std::chrono::steady_clock::time_point timestamp,
    ClusterShardHostInfos infos, Password password,
    const std::shared_ptr<engine::ev::ThreadPool>& /*redis_thread_pool*/,
    const NodesStorage& nodes)
    : infos_(std::move(infos)),
      password_(std::move(password)),
      version_(version),
      timestamp_(timestamp) {
  {
    size_t all_instances_count = 0;
    for (const auto& info : infos_) {
      /// +1 for master
      all_instances_count += info.slaves.size() + 1;
    }

    auto HostPortToString = [](const std::pair<std::string, int>& host_port) {
      return host_port.first + ":" + std::to_string(host_port.second);
    };
    cluster_shards_.reserve(infos_.size());

    ClusterShard::RedisConnectionPtr super_master;
    std::vector<ClusterShard::RedisConnectionPtr> super_replicas;
    super_replicas.reserve(all_instances_count);

    for (const auto& info : infos_) {
      const auto current_shard = cluster_shards_.size();
      const auto& master_host_port = HostPortToString(info.master.HostPort());
      /// Throws rcu::MissingKeyException on missing key in nodes
      std::shared_ptr<const RedisConnectionHolder> master =
          nodes[master_host_port];
      std::vector<std::shared_ptr<const RedisConnectionHolder>> replicas;
      replicas.reserve(info.slaves.size());
      for (const auto& replica : info.slaves) {
        auto host_port = HostPortToString(replica.HostPort());
        /// Throws rcu::MissingKeyException on missing key in nodes
        replicas.push_back(nodes[host_port]);
        host_port_to_shard_[host_port] = current_shard;
      }
      host_port_to_shard_[master_host_port] = current_shard;

      if (!super_master) {
        super_master = master;
      } else {
        super_replicas.push_back(master);
      }
      super_replicas.insert(super_replicas.end(), replicas.begin(),
                            replicas.end());
      cluster_shards_.emplace_back(current_shard, std::move(master),
                                   std::move(replicas));
    }
    super_shard_ = ClusterShard(kUnknownShard, std::move(super_master),
                                std::move(super_replicas));
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

void ClusterTopology::GetStatistics(const MetricsSettings& settings,
                                    SentinelStatistics& stats) const {
  size_t shard_index = 0;
  for (const auto& shard : cluster_shards_) {
    const auto& shard_name = GetShardName(shard_index++);
    auto master_it =
        stats.masters.emplace(shard_name, ShardStatistics(settings));
    auto& master_stats = master_it.first->second;
    shard.GetStatistics(true, settings, master_stats);

    auto replica_it =
        stats.slaves.emplace(shard_name, ShardStatistics(settings));
    auto& replica_stats = replica_it.first->second;
    shard.GetStatistics(false, settings, replica_stats);

    stats.shard_group_total.Add(master_stats.shard_total);
    stats.shard_group_total.Add(replica_stats.shard_total);
  }
}

std::unordered_map<ServerId, size_t, ServerIdHasher>
ClusterTopology::GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                                             const CommandControl& cc) const {
  if (shard_idx == kUnknownShard) {
    return super_shard_.GetAvailableServersWeighted(with_master, cc);
  }
  return cluster_shards_.at(shard_idx).GetAvailableServersWeighted(with_master,
                                                                   cc);
}

const std::string& GetShardName(size_t shard_index) {
  static const size_t kMaxClusterShards = 500;
  static const std::vector<std::string> names = [] {
    std::vector<std::string> shard_names;
    shard_names.reserve(kMaxClusterShards);
    for (size_t i = 0; i < kMaxClusterShards; ++i) {
      auto number = std::to_string(i);
      if (number.size() < 2) {
        number.insert(0, "0");
      }
      auto name = "shard" + number;
      shard_names.push_back(std::move(name));
    }
    return shard_names;
  }();
  return names.at(shard_index);
}

}  // namespace redis

USERVER_NAMESPACE_END
