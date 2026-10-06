#pragma once
#include <storages/redis/impl/topology_holder_base.hpp>
#include <userver/engine/pulse_event.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/storages/redis/redis_state.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/periodic_watcher.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class RedisConnectionHolder;

class SentinelTopologyHolder final : public TopologyHolderBase {
public:
    using HostPort = std::string;

    /// TODO: Need to pass sentinel_password as well
    SentinelTopologyHolder(
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        const std::string& shard_group_name,
        const Credentials& credentials,
        std::size_t database_index,
        const std::vector<std::string>& shard_names,
        const std::vector<ConnectionInfo>& conns,
        ConnectionSecurity connection_security
    );

    ~SentinelTopologyHolder() override;

    void Init() override;

    void Start() override;

    void Stop() override;

    bool WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) override;
    bool IsReady(const HealthCheckParams& params) const override;

    rcu::ReadablePtr<ClusterTopology, rcu::ExclusiveRcuTraits> GetTopology() const override;

    void SendUpdateClusterTopology() override;

    std::shared_ptr<Redis> GetRedisInstance(const HostPort& host_port) const override;

    void GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const override;

    void SetCommandsBufferingSettings(CommandsBufferingSettings settings) override;

    void SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) override;

    void SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) override;

    void SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) override;

    boost::signals2::signal<void(HostPort, Redis::State)>& GetSignalNodeStateChanged() override;

    boost::signals2::signal<void(size_t)>& GetSignalTopologyChanged() override;

    void UpdateCredentials(const Credentials& credentials) override;

    Credentials GetCredentials() override;

    std::string GetReadinessInfo() const override;

private:
    std::shared_ptr<RedisConnectionHolder> CreateRedisInstance(const HostPort& host_port);

    engine::ev::ThreadControl ev_thread_;
    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
    const std::string shard_group_name_;
    logging::LogExtra log_extra_;
    Credentials credentials_;
    const std::size_t database_index_;

    // maps shard_name to shard_idx
    std::unordered_map<std::string, size_t> shard_by_name_;
    std::vector<std::string> name_by_shard_;
    const std::vector<ConnectionInfo> conns_;
    StatisticsHolder statistics_holder_;

    /// Update cluster topology
    /// @{
    engine::ev::PeriodicWatcher update_topology_timer_;
    engine::ev::AsyncWatcher update_topology_watch_;
    void UpdateClusterTopology();

    engine::ev::AsyncWatcher create_instances_and_update_topology_watch_;
    void CreateInstancesAndUpdateTopology();
    /// @}

    engine::ev::PeriodicWatcher sentinels_process_creation_timer_;
    engine::ev::AsyncWatcher sentinels_process_creation_watch_;
    engine::ev::AsyncWatcher sentinels_process_state_update_watch_;
    bool first_entry_point_connected_{false};

    ///{ Wait ready
    engine::PulseEvent readiness_event_;
    std::atomic<bool> is_topology_received_{false};
    ///}

    std::optional<CommandsBufferingSettings> commands_buffering_settings_;
    ReplicationMonitoringSettings monitoring_settings_;
    utils::RetryBudgetSettings retry_budget_settings_;

    boost::signals2::signal<void(HostPort, Redis::State)> signal_node_state_change_;
    boost::signals2::signal<void(size_t shards_count)> signal_topology_changed_;
    std::shared_ptr<std::atomic<bool>> update_topology_flag_{std::make_shared<std::atomic<bool>>(false)};
    std::shared_ptr<utils::ScopeGuard> update_topology_guard_;
    ClusterShardHostInfos new_shard_host_info_;
    NodesStorage nodes_;

    std::shared_ptr<Shard> sentinels_;
    std::unordered_map<HostPort, utils::datetime::SteadyCoarseClock::time_point> nodes_last_seen_time_;
    rcu::RcuMap<std::string, std::string, SingleWriterRcuMapTraits<std::string>> ip_by_fqdn_;

    /// TODO: replace with relaxed counters
    std::atomic_size_t current_topology_version_{0};
    std::atomic_size_t cluster_slots_call_counter_{0};
    rcu::Variable<ClusterTopology, rcu::ExclusiveRcuTraits> topology_;

    const ConnectionSecurity connection_security_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
