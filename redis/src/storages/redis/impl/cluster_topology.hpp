#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <vector>

#include <userver/rcu/rcu_map.hpp>
#include <userver/storages/redis/impl/base.hpp>

#include <storages/redis/impl/cluster_shard.hpp>
#include <storages/redis/impl/sentinel_query.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

template <typename K, typename V>
struct StdMutexRcuMapTraits : rcu::DefaultRcuMapTraits<K, V> {
  using MutexType = std::mutex;
};

using NodesStorage =
    rcu::RcuMap<std::string, Redis, StdMutexRcuMapTraits<std::string, Redis>>;

class ClusterTopology {
 public:
  ClusterTopology() = default;
  ClusterTopology(ClusterTopology&&) = default;
  ClusterTopology(const ClusterTopology&) = default;
  ClusterTopology& operator=(ClusterTopology&&) = default;
  ClusterTopology& operator=(const ClusterTopology&) = default;

  ClusterTopology(
      size_t version, std::chrono::steady_clock::time_point timestamp,
      ClusterShardHostInfos infos, Password password,
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
      NodesStorage& nodes);
  ~ClusterTopology();

  size_t GetShardIndexBySlot(uint16_t slot) const {
    return slot_to_shard_.at(slot);
  }

  ClusterShard GetClusterShardByIndex(uint16_t index) const {
    return cluster_shards_.at(index);
  }

  bool IsReady(WaitConnectedMode mode) const;

  bool HasSameInfos(const ClusterShardHostInfos& infos) const;

  const ClusterShardHostInfos& GetShardInfos() const { return infos_; }
  size_t GetVersion() const { return version_; }
  size_t GetShardsCount() const { return cluster_shards_.size(); }
  std::chrono::steady_clock::time_point GetTimestamp() const {
    return timestamp_;
  }

  void GetStatistics(const MetricsSettings& settings, SentinelStatistics& stats,
                     const std::vector<std::string>& shard_names) const;

 private:
  class ShardReadiness {
   public:
    bool IsReady() const { return master_ready_ && replica_ready_; }
    void SetMasterReady() { master_ready_ = true; }
    void SetReplicaReady() { replica_ready_ = true; }

   private:
    std::atomic_bool master_ready_{false};
    std::atomic_bool replica_ready_{false};
  };

  ClusterShardHostInfos infos_;
  Password password_;
  std::array<uint16_t, kClusterHashSlots> slot_to_shard_{};

  std::vector<ClusterShard> cluster_shards_;
  size_t version_ = 0;
  std::chrono::steady_clock::time_point timestamp_{};
};

}  // namespace redis

USERVER_NAMESPACE_END
