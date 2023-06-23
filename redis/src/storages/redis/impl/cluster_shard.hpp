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
#include <storages/redis/impl/sentinel_query.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

class ClusterShard {
 public:
  using RedisPtr = std::shared_ptr<redis::Redis>;

  ClusterShard(size_t shard, RedisPtr master, std::vector<RedisPtr> replicas)
      : master_(std::move(master)),
        replicas_(std::move(replicas)),
        shard_(shard) {}
  ClusterShard(const ClusterShard& other)
      : master_(other.master_),
        replicas_(other.replicas_),
        current_(other.current_.load()),
        shard_(other.shard_) {}
  ClusterShard(ClusterShard&& other) noexcept
      : master_(std::move(other.master_)),
        replicas_(std::move(other.replicas_)),
        current_(other.current_.load()),
        shard_(other.shard_) {}
  ClusterShard& operator=(const ClusterShard& other);
  ClusterShard& operator=(ClusterShard&& other) noexcept;
  bool IsReady(WaitConnectedMode mode) const;
  bool AsyncCommand(CommandPtr command);
  ShardStatistics GetStatistics(bool master,
                                const MetricsSettings& settings) const;

 private:
  /// Get instance id, just to check if command was already executed on this
  /// instance. Returned number is not any index, just some id
  static size_t ToInstanceIdx(const RedisPtr& ptr) {
    return reinterpret_cast<size_t>(ptr.get());
  }

  static void GetNearestServersPing(const CommandControl& command_control,
                                    std::vector<RedisPtr>& instances);
  static std::vector<RedisPtr> GetAvailableServers(
      const RedisPtr& master, const std::vector<RedisPtr>& replicas,
      const CommandControl& command_control, bool with_masters,
      bool with_slaves);
  RedisPtr GetInstance(const std::vector<RedisPtr>& instances, size_t skip_idx);
  bool IsMasterReady() const;
  bool IsReplicaReady() const;

  RedisPtr master_;
  std::vector<RedisPtr> replicas_;
  std::atomic_size_t current_{0};
  size_t shard_;
};

}  // namespace redis

USERVER_NAMESPACE_END
