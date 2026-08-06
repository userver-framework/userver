#pragma once
#include <memory>

#include <engine/ev/watcher/periodic_watcher.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/redis_creation_settings.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/sentinel_impl.hpp>
#include <storages/redis/impl/statistics_holder.hpp>
#include <userver/concurrent/variable.hpp>
#include <userver/rcu/rcu.hpp>

#include "hysteresis.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

/// This class holds redis connection and automatically reconnects if
/// disconnected
class RedisConnectionHolder final : public std::enable_shared_from_this<RedisConnectionHolder> {
    class EmplaceEnabler;

public:
    static constexpr redis::RedisCreationSettings makeClusterNodeRedisCreationSettings() {
        /// Here we allow read from replicas possibly stale data.
        /// This does not affect connections to masters
        return redis::RedisCreationSettings{ConnectionSecurity::kNone, true};
    }
    static constexpr redis::RedisCreationSettings makeSentinelNodeRedisCreationSettings() {
        /// Non-cluster nodes does not support READONLY command
        return redis::RedisCreationSettings{ConnectionSecurity::kNone, false};
    }

    RedisConnectionHolder() = delete;
    RedisConnectionHolder(
        EmplaceEnabler,
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        const std::string& shard_group_name,
        const std::string& host,
        uint16_t port,
        Credentials credentials,
        std::size_t database_index,
        CommandsBufferingSettings buffering_settings,
        ReplicationMonitoringSettings replication_monitoring_settings,
        utils::RetryBudgetSettings retry_budget_settings,
        redis::RedisCreationSettings redis_creation_settings,
        StatisticsHolder::Stats& stats,
        Hysteresis::Config config = Hysteresis::Config(),
        std::chrono::seconds max_disconnect_time = std::chrono::seconds(35)
    );
    ~RedisConnectionHolder();
    RedisConnectionHolder(const RedisConnectionHolder&) = delete;
    RedisConnectionHolder& operator=(const RedisConnectionHolder&) = delete;

    static std::shared_ptr<RedisConnectionHolder> Create(
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        const std::string& shard_group_name,
        const std::string& host,
        uint16_t port,
        Credentials credentials,
        std::size_t database_index,
        CommandsBufferingSettings buffering_settings,
        ReplicationMonitoringSettings replication_monitoring_settings,
        utils::RetryBudgetSettings retry_budget_settings,
        StatisticsHolder::Stats& stats,
        redis::RedisCreationSettings redis_creation_settings = makeClusterNodeRedisCreationSettings(),
        Hysteresis::Config config = Hysteresis::Config(),
        std::chrono::seconds max_disconnect_time = std::chrono::seconds(35)
    );

    std::shared_ptr<Redis> Get() const;

    void SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings);
    void SetCommandsBufferingSettings(CommandsBufferingSettings settings);
    void SetRetryBudgetSettings(utils::RetryBudgetSettings settings);

    Redis::State GetState() const;
    size_t GetCommandsCounter() const;

    // Failed state control using external source (for example CLUSTER NODES)
    void SetFailed(bool failed) noexcept { failed_hysteresis_.SetFailed(failed); }
    void AccountFail(bool fail) noexcept { failed_hysteresis_.AccountFail(fail); }

    // Get the health state of the host, taking into account the connection state and other data.
    bool IsReady() const noexcept;

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    boost::signals2::signal<void(Redis::State)> signal_state_change;

private:
    void CreateConnection();
    /// Checks if redis connected. If not recreate connection
    void EnsureConnected();
    void OnStateChanged(Redis::State state);

    concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex> commands_buffering_settings_;
    concurrent::Variable<ReplicationMonitoringSettings, std::mutex> replication_monitoring_settings_;
    concurrent::Variable<utils::RetryBudgetSettings, std::mutex> retry_budget_settings_;
    engine::ev::ThreadControl ev_thread_;
    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
    const std::string shard_group_name_;
    const std::string host_;
    const uint16_t port_;
    const Credentials credentials_;
    const std::size_t database_index_;
    Statistics& statistics_;
    rcu::Variable<std::shared_ptr<Redis>, rcu::BlockingRcuTraits> redis_;
    engine::ev::PeriodicWatcher connection_check_timer_;
    const RedisCreationSettings redis_creation_settings_;

    /// Responsible for tracking the failed state of the host we are connecting to.
    /// This information is obtained from an external source - from other connections.
    Hysteresis failed_hysteresis_;
    std::chrono::steady_clock::time_point disconnected_time_;
    const std::chrono::seconds max_disconnect_time_{35};
    bool was_ever_connected_{false};
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
