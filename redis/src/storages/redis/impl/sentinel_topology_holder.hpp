#pragma once
#include <storages/redis/impl/topology_holder_base.hpp>
#include <userver/concurrent/variable.hpp>
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
        const Password& password,
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
    std::shared_ptr<RedisConnectionHolder> CreateRedisInstance(const HostPort& host_port);

    engine::ev::ThreadControl ev_thread_;
    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;
    const std::string shard_group_name_;
    logging::LogExtra log_extra_;
    concurrent::Variable<Password, std::mutex> password_;
    const std::size_t database_index_;

    // maps shard_name to shard_idx
    std::unordered_map<std::string, size_t> shard_by_name_;
    std::vector<std::string> name_by_shard_;
    rcu::Variable<std::vector<ConnectionInfo>> conns_;

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
    std::atomic<bool> first_entry_point_connected_{false};

    ///{ Wait ready
    std::mutex mutex_;
    engine::impl::ConditionVariableAny<std::mutex> cv_;
    std::atomic<bool> is_topology_received_{false};
    bool IsInitialized() const { return is_topology_received_.load(); }
    ///}

    concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex> commands_buffering_settings_;
    concurrent::Variable<ReplicationMonitoringSettings, std::mutex> monitoring_settings_;
    concurrent::Variable<utils::RetryBudgetSettings, std::mutex> retry_budget_settings_;

    boost::signals2::signal<void(HostPort, Redis::State)> signal_node_state_change_;
    boost::signals2::signal<void(size_t shards_count)> signal_topology_changed_;
    std::atomic<bool> update_topology_flag_{false};
    std::shared_ptr<utils::ScopeGuard> update_topology_guard_;
    concurrent::Variable<ClusterShardHostInfos, std::mutex> new_shard_host_info_;
    NodesStorage nodes_;

    std::shared_ptr<Shard> sentinels_;
    std::unordered_map<HostPort, utils::datetime::SteadyCoarseClock::time_point> nodes_last_seen_time_;
    rcu::RcuMap<std::string, std::string, StdMutexRcuMapTraits<std::string>> ip_by_fqdn_;

    /// TODO: replace with relaxed counters
    std::atomic_size_t current_topology_version_{0};
    std::atomic_size_t cluster_slots_call_counter_{0};
    rcu::Variable<ClusterTopology, rcu::BlockingRcuTraits> topology_;

    const ConnectionSecurity connection_security_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
