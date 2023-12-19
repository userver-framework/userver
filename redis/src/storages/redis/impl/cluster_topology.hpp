#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include <userver/rcu/rcu_map.hpp>
#include <userver/storages/redis/impl/base.hpp>

#include <storages/redis/impl/cluster_shard.hpp>
#include <storages/redis/impl/sentinel_impl.hpp>
#include <storages/redis/impl/sentinel_query.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

template <typename K, typename V>
struct StdMutexRcuMapTraits : rcu::DefaultRcuMapTraits<K, V> {
  using MutexType = std::mutex;
};

class RedisConnectionHolder;
using NodesStorage =
    rcu::RcuMap<std::string, RedisConnectionHolder,
                StdMutexRcuMapTraits<std::string, RedisConnectionHolder>>;

class ClusterTopology {
 public:
  static constexpr auto kUnknownShard = SentinelImplBase::kUnknownShard;

  ClusterTopology() = default;
  ClusterTopology(ClusterTopology&&) = default;
  ClusterTopology(const ClusterTopology&) = default;
  ClusterTopology& operator=(ClusterTopology&&) = default;
  ClusterTopology& operator=(const ClusterTopology&) = default;

  ClusterTopology(
      size_t version, std::chrono::steady_clock::time_point timestamp,
      ClusterShardHostInfos infos, Password password,
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
      const NodesStorage& nodes);
  ~ClusterTopology();

  size_t GetShardIndexBySlot(uint16_t slot) const {
    return slot_to_shard_.at(slot);
  }

  std::optional<size_t> GetShardByHostPort(const std::string& host_port) const {
    auto it = host_port_to_shard_.find(host_port);
    if (it == host_port_to_shard_.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  const ClusterShard& GetClusterShardByIndex(size_t index) const {
    if (index == kUnknownShard) {
      return super_shard_;
    }
    if (index >= GetShardsCount()) {
      /// Choose safe index in case of decreased shards count since command
      /// initiated. Return "supershard" so make request to random host, then on
      /// MOVED we will do retry to correct host.
      return super_shard_;
    }
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

  void GetStatistics(const MetricsSettings& settings,
                     SentinelStatistics& stats) const;

  std::unordered_map<ServerId, size_t, ServerIdHasher>
  GetAvailableServersWeighted(size_t shard_idx, bool with_master,
                              const CommandControl& cc) const;

 private:
  ClusterShardHostInfos infos_;
  Password password_;
  std::array<uint16_t, kClusterHashSlots> slot_to_shard_{};

  /// Special "Shard" containing all instances of cluster, master - is 0-shard
  /// master.
  ClusterShard super_shard_;
  std::vector<ClusterShard> cluster_shards_;
  std::unordered_map<std::string, size_t> host_port_to_shard_{};
  size_t version_ = 0;
  std::chrono::steady_clock::time_point timestamp_{};
};

const std::string& GetShardName(size_t shard_index);

}  // namespace redis

USERVER_NAMESPACE_END
