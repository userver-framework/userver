#pragma once
#include <memory>

#include <engine/ev/watcher/periodic_watcher.hpp>
#include <storages/redis/impl/cluster_sentinel_impl.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

/// This class holds redis connection and automaticaly reconnects if
/// disconnected
class RedisConnectionHolder {
 public:
  RedisConnectionHolder(
      const engine::ev::ThreadControl& sentinel_thread_control,
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
      const std::string& host, uint16_t port, Password password,
      CommandsBufferingSettings buffering_settings,
      ReplicationMonitoringSettings replication_monitoring_settings);
  ~RedisConnectionHolder();
  RedisConnectionHolder(const RedisConnectionHolder&) = delete;
  RedisConnectionHolder(RedisConnectionHolder&&) noexcept = default;
  RedisConnectionHolder& operator=(const RedisConnectionHolder&) = delete;
  RedisConnectionHolder& operator=(RedisConnectionHolder&&) noexcept = default;

  std::shared_ptr<Redis> Get() const;
  /// Stats GetStats() const;

  void SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings);
  void SetCommandsBufferingSettings(CommandsBufferingSettings settings);

  Redis::State GetState() const;

 private:
  void CreateConnection();
  /// Checks if redis connected. If not recreate connection
  void EnsureConnected();

  concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex>
      commands_buffering_settings_;
  concurrent::Variable<ReplicationMonitoringSettings, std::mutex>
      replication_monitoring_settings_;
  engine::ev::ThreadControl ev_thread_;
  std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
  const std::string host_;
  const uint16_t port_;
  const Password password_;
  rcu::Variable<std::shared_ptr<Redis>, StdMutexRcuTraits> redis_;
  engine::ev::PeriodicWatcher connection_check_timer_;
};

}  // namespace redis

USERVER_NAMESPACE_END
