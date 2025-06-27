#pragma once

#include <storages/redis/impl/topology_holder_base.hpp>
#include <userver/concurrent/variable.hpp>
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
        const Password& password,
        std::size_t database_index,
        ConnectionInfo conn
    );

    ~StandaloneTopologyHolder() override;

    void Init() override;

    void Start() override;

    void Stop() override;

    bool WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) override;

    rcu::ReadablePtr<ClusterTopology, rcu::BlockingRcuTraits> GetTopology() const override;

    void SendUpdateClusterTopology() override;

    std::shared_ptr<Redis> GetRedisInstance(const HostPort& host_port) const override;

    void GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const override;

    void SetCommandsBufferingSettings(CommandsBufferingSettings settings) override;

    void SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) override;

    void SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) override;

    void SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) override;

    boost::signals2::signal<void(HostPort, Redis::State)>& GetSignalNodeStateChanged() override;

    boost::signals2::signal<void(size_t)>& GetSignalTopologyChanged() override;

    void UpdatePassword(const Password& password) override;

    Password GetPassword() override;

    std::string GetReadinessInfo() const override;

private:
    std::shared_ptr<RedisConnectionHolder> CreateRedisInstance(const ConnectionInfoInt& info);

    void CreateNode();

    struct Node {
        HostPort host_port;
        std::shared_ptr<RedisConnectionHolder> node;
    };

    engine::ev::ThreadControl ev_thread_;
    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
    concurrent::Variable<Password, std::mutex> password_;
    const std::size_t database_index_;

    ///{ Wait ready
    std::mutex mutex_;
    engine::impl::ConditionVariableAny<std::mutex> cv_;
    ConnectionInfoInt conn_to_create_;
    std::atomic<bool> is_nodes_received_{false};

    rcu::Variable<std::optional<Node>, rcu::BlockingRcuTraits> node_;
    rcu::Variable<ClusterTopology, rcu::BlockingRcuTraits> topology_;
    std::atomic_size_t current_topology_version_{0};

    engine::ev::AsyncWatcher create_node_watch_;

    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes)
    boost::signals2::signal<void(HostPort, Redis::State)> signal_node_state_change_;
    boost::signals2::signal<void(size_t shards_count)> signal_topology_changed_;

    concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex> commands_buffering_settings_;
    concurrent::Variable<ReplicationMonitoringSettings, std::mutex> monitoring_settings_;
    concurrent::Variable<utils::RetryBudgetSettings, std::mutex> retry_budget_settings_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
