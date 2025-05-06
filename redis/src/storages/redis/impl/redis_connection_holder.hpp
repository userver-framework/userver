#pragma once
#include <memory>

#include <engine/ev/watcher/periodic_watcher.hpp>
#include <storages/redis/impl/cluster_sentinel_impl.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/redis_creation_settings.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

/// This class holds redis connection and automatically reconnects if
/// disconnected
class RedisConnectionHolder final : public std::enable_shared_from_this<RedisConnectionHolder> {
    class EmplaceEnabler;

public:
    static constexpr redis::RedisCreationSettings makeDefaultRedisCreationSettings() {
        /// Here we allow read from replicas possibly stale data.
        /// This does not affect connections to masters
        return redis::RedisCreationSettings{ConnectionSecurity::kNone, true};
    }

    RedisConnectionHolder() = delete;
    RedisConnectionHolder(
        EmplaceEnabler,
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        const std::string& host,
        uint16_t port,
        Password password,
        std::size_t database_index,
        CommandsBufferingSettings buffering_settings,
        ReplicationMonitoringSettings replication_monitoring_settings,
        utils::RetryBudgetSettings retry_budget_settings,
        redis::RedisCreationSettings redis_creation_settings
    );
    ~RedisConnectionHolder();
    RedisConnectionHolder(const RedisConnectionHolder&) = delete;
    RedisConnectionHolder& operator=(const RedisConnectionHolder&) = delete;

    static std::shared_ptr<RedisConnectionHolder> Create(
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        const std::string& host,
        uint16_t port,
        Password password,
        std::size_t database_index,
        CommandsBufferingSettings buffering_settings,
        ReplicationMonitoringSettings replication_monitoring_settings,
        utils::RetryBudgetSettings retry_budget_settings,
        redis::RedisCreationSettings redis_creation_settings = makeDefaultRedisCreationSettings()
    );

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

    concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex> commands_buffering_settings_;
    concurrent::Variable<ReplicationMonitoringSettings, std::mutex> replication_monitoring_settings_;
    concurrent::Variable<utils::RetryBudgetSettings, std::mutex> retry_budget_settings_;
    engine::ev::ThreadControl ev_thread_;
    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
    const std::string host_;
    const uint16_t port_;
    const Password password_;
    const std::size_t database_index_;
    rcu::Variable<std::shared_ptr<Redis>, rcu::BlockingRcuTraits> redis_;
    engine::ev::PeriodicWatcher connection_check_timer_;
    const RedisCreationSettings redis_creation_settings_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
