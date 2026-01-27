#pragma once

#include <engine/ev/watcher.hpp>
#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/periodic_watcher.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <storages/redis/impl/topology_holder_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

constexpr auto kProcessCreationInterval = std::chrono::seconds(3);
constexpr auto kDeleteNodesCheckInterval = std::chrono::seconds(60);
constexpr auto kDeleteNodeInterval = std::chrono::seconds(600);

class ClusterTopologyHolder final : public TopologyHolderBase {
public:
    using HostPort = std::string;

    ClusterTopologyHolder(
        const engine::ev::ThreadControl& sentinel_thread_control,
        const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
        std::string shard_group_name,
        Password password,
        const std::vector<std::string>& /*shards*/,
        const std::vector<ConnectionInfo>& conns,
        ConnectionSecurity connection_security
    );

    ~ClusterTopologyHolder() override = default;

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

    static size_t GetClusterSlotsCalledCounter() { return cluster_slots_call_counter.load(std::memory_order_relaxed); }

private:
    std::shared_ptr<RedisConnectionHolder> CreateRedisInstance(const HostPort& host_port);

    engine::ev::ThreadControl ev_thread_;
    std::shared_ptr<engine::ev::ThreadPool> redis_thread_pool_;

    const std::string shard_group_name_;
    logging::LogExtra log_extra_;
    concurrent::Variable<Password, std::mutex> password_;
    std::shared_ptr<const std::vector<std::string>> shards_names_;
    const std::vector<ConnectionInfo> conns_;
    std::shared_ptr<Shard> sentinels_;

    std::atomic_size_t current_topology_version_{0};
    rcu::Variable<ClusterTopology, rcu::BlockingRcuTraits> topology_;

    /// Update cluster topology
    /// @{
    engine::ev::PeriodicWatcher update_topology_timer_;
    engine::ev::AsyncWatcher update_topology_watch_;
    void UpdateClusterTopology();
    /// @}

    /// Discover actual nodes in cluster
    engine::ev::AsyncWatcher explore_nodes_watch_;
    engine::ev::PeriodicWatcher explore_nodes_timer_;
    std::atomic<bool> first_entry_point_connected_{false};
    void ExploreNodes();

    /// Create connections to discovered nodes
    engine::ev::AsyncWatcher create_nodes_watch_;
    void CreateNodes();

    /// Delete non-actual nodes
    engine::ev::PeriodicWatcher delete_expired_nodes_timer_;
    void DeleteNodes();

    engine::ev::PeriodicWatcher sentinels_process_creation_timer_;
    engine::ev::AsyncWatcher sentinels_process_creation_watch_;
    engine::ev::AsyncWatcher sentinels_process_state_update_watch_;

    ///{ Wait ready
    std::mutex mutex_;
    engine::impl::ConditionVariableAny<std::mutex> cv_;
    std::atomic<bool> is_topology_received_{false};
    std::atomic<bool> is_nodes_received_{false};
    std::atomic<bool> update_cluster_slots_flag_{false};
    bool IsInitialized() const { return is_nodes_received_.load() && is_topology_received_.load(); }
    ///}

    boost::signals2::signal<void(HostPort, Redis::State)> signal_node_state_change_;
    boost::signals2::signal<void(size_t shards_count)> signal_topology_changed_;
    NodesStorage nodes_;

    concurrent::Variable<std::optional<CommandsBufferingSettings>, std::mutex> commands_buffering_settings_;
    concurrent::Variable<ReplicationMonitoringSettings, std::mutex> monitoring_settings_;
    concurrent::Variable<utils::RetryBudgetSettings, std::mutex> retry_budget_settings_;
    concurrent::Variable<std::unordered_set<HostPort>, std::mutex> nodes_to_create_;
    concurrent::Variable<std::unordered_set<HostPort>, std::mutex> actual_nodes_;
    // work only from sentinel thread so no need to synchronize it
    std::unordered_map<HostPort, utils::datetime::SteadyCoarseClock::time_point> nodes_last_seen_time_;
    rcu::RcuMap<std::string, std::string, StdMutexRcuMapTraits<std::string>> ip_by_fqdn_;

    static std::atomic<size_t> cluster_slots_call_counter;

    const ConnectionSecurity connection_security_;
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
