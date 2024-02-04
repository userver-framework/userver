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

/// This class holds redis connection and automatically reconnects if
/// disconnected
class RedisConnectionHolder
    : public std::enable_shared_from_this<RedisConnectionHolder> {
 public:
  RedisConnectionHolder(
      const engine::ev::ThreadControl& sentinel_thread_control,
      const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
      const std::string& host, uint16_t port, Password password,
      CommandsBufferingSettings buffering_settings,
      ReplicationMonitoringSettings replication_monitoring_settings,
      utils::RetryBudgetSettings retry_budget_settings);
  ~RedisConnectionHolder();
  RedisConnectionHolder(const RedisConnectionHolder&) = delete;
  RedisConnectionHolder& operator=(const RedisConnectionHolder&) = delete;

  std::shared_ptr<Redis> Get() const;

  void SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings);
  void SetCommandsBufferingSettings(CommandsBufferingSettings settings);
  void SetRetryBudgetSettings(utils::RetryBudgetSettings settings);

  Redis::State GetState() const;

  // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
  boost::signals2::signal<void(Redis::State)> signal_state_change;

 private:
  void CreateConnection();
  /// Checks if redis connected. If not recreate connection
  void EnsureConnected();

  concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex>
      commands_buffering_settings_;
  concurrent::Variable<ReplicationMonitoringSettings, std::mutex>
      replication_monitoring_settings_;
  concurrent::Variable<utils::RetryBudgetSettings, std::mutex>
      retry_budget_settings_;
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
