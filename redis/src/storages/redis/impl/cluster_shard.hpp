#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <vector>

#include <userver/rcu/rcu_map.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/types.hpp>
#include <userver/storages/redis/impl/wait_connected_mode.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/redis_connection_holder.hpp>
#include <storages/redis/impl/sentinel_query.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class ClusterShard {
 public:
  using RedisPtr = std::shared_ptr<redis::Redis>;
  using RedisConnectionPtr = std::shared_ptr<const RedisConnectionHolder>;
  using ServersWeighted = std::unordered_map<ServerId, size_t, ServerIdHasher>;

  ClusterShard() = default;
  ClusterShard(size_t shard, RedisConnectionPtr master,
               std::vector<RedisConnectionPtr> replicas)
      : replicas_(std::move(replicas)),
        master_(std::move(master)),
        shard_(shard) {}
  ClusterShard(const ClusterShard& other)
      : replicas_(other.replicas_),
        master_(other.master_),
        current_(other.current_.load()),
        shard_(other.shard_) {}
  ClusterShard(ClusterShard&& other) noexcept
      : replicas_(std::move(other.replicas_)),
        master_(std::move(other.master_)),
        current_(other.current_.load()),
        shard_(other.shard_) {}
  ClusterShard& operator=(const ClusterShard& other);
  ClusterShard& operator=(ClusterShard&& other) noexcept;
  bool IsReady(WaitConnectedMode mode) const;
  bool AsyncCommand(CommandPtr command) const;
  ShardStatistics GetStatistics(bool master,
                                const MetricsSettings& settings) const;

  ServersWeighted GetAvailableServersWeighted(
      bool with_master, const CommandControl& command_control) const;

 private:
  static void GetNearestServersPing(const CommandControl& command_control,
                                    std::vector<RedisConnectionPtr>& instances);
  /// Return suitable instance if it is the only suitable instance.
  /// If there no suitable or multiple suitable instances then method return
  /// nullptr
  RedisPtr GetAvailableServer(const CommandControl& command_control,
                              bool read_only) const;
  std::vector<RedisConnectionPtr> GetAvailableServers(
      const CommandControl& command_control) const;
  static RedisPtr GetInstance(const std::vector<RedisConnectionPtr>& instances,
                              size_t start_idx, size_t attempt,
                              bool is_nearest_ping_server, size_t best_dc_count,
                              size_t* pinstance_idx);
  std::vector<RedisConnectionPtr> MakeReadonlyWithMasters() const;
  bool IsMasterReady() const;
  bool IsReplicaReady() const;

  std::vector<RedisConnectionPtr> replicas_;
  RedisConnectionPtr master_;
  mutable std::atomic_size_t current_{0};
  size_t shard_{0};
};

size_t GetStartIndex(const CommandControl& command_control, size_t attempt,
                     bool is_nearest_ping_server, size_t prev_instance_idx,
                     size_t current, size_t servers_count);
}  // namespace redis

USERVER_NAMESPACE_END
