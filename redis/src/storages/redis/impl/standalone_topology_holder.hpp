#pragma once

#include <storages/redis/impl/connection_kind.hpp>
#include <storages/redis/impl/topology_holder_base.hpp>
#include <userver/engine/pulse_event.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/redis/redis_state.hpp>

#include <engine/ev/watcher/async_watcher.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class RedisConnectionHolder;

class StandaloneTopologyHolder final : public TopologyHolderBase {
public:
    StandaloneTopologyHolder(
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        const std::string& shard_group_name,
        const Credentials& credentials,
        std::size_t database_index,
        ConnectionInfo conn,
        ConnectionMode connection_mode = ConnectionMode::kCommands
    );

    ~StandaloneTopologyHolder() override;

    void Init() override;

    void Start() override;

    void Stop() override;

    bool WaitReadyOnce(
        engine::Deadline deadline,
        WaitConnectedMode mode,
        ConnectionKind kind = ConnectionKind::kCommand
    ) override;
    bool IsReady(const HealthCheckParams& params, ConnectionKind kind = ConnectionKind::kCommand) const override;

    rcu::ReadablePtr<ClusterTopology, rcu::ExclusiveRcuTraits> GetTopology() const override;

    void SendUpdateClusterTopology() override;

    std::shared_ptr<Redis> GetRedisInstance(const HostPort& host_port, ConnectionKind kind) const override;

    void GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const override;

    void SetCommandsBufferingSettings(CommandsBufferingSettings settings) override;

    void SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) override;

    void SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) override;

    void SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) override;

    boost::signals2::signal<void(HostPort, Redis::State)>& GetSignalNodeStateChanged() override;

    boost::signals2::signal<void(size_t)>& GetSignalTopologyChanged() override;

    void UpdateCredentials(const Credentials& credentials) override;

    Credentials GetCredentials() override;

    std::string GetReadinessInfo(ConnectionKind kind = ConnectionKind::kCommand) const override;

private:
    std::shared_ptr<RedisConnectionHolder> CreateRedisInstance(const ConnectionInfoInt& info);

    void CreateNode();

    struct Node {
        HostPort host_port;
        std::shared_ptr<RedisConnectionHolder> node;
    };

    engine::ev::ThreadControl ev_thread_;
    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
    const std::string shard_group_name_;
    Credentials credentials_;
    const std::size_t database_index_;
    const ConnectionMode connection_mode_;

    ///{ Wait ready
    engine::PulseEvent readiness_event_;
    ConnectionInfoInt conn_to_create_;
    std::atomic<bool> is_nodes_received_{false};

    StatisticsHolder statistics_holder_;
    rcu::Variable<std::optional<Node>, rcu::ExclusiveRcuTraits> node_;
    rcu::Variable<ClusterTopology, rcu::ExclusiveRcuTraits> topology_;
    std::atomic_size_t current_topology_version_{0};

    engine::ev::AsyncWatcher create_node_watch_;

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    boost::signals2::signal<void(HostPort, Redis::State)> signal_node_state_change_;
    boost::signals2::signal<void(size_t shards_count)> signal_topology_changed_;

    std::optional<CommandsBufferingSettings> commands_buffering_settings_;
    ReplicationMonitoringSettings monitoring_settings_;
    utils::RetryBudgetSettings retry_budget_settings_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
