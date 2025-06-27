#include <storages/redis/impl/standalone_topology_holder.hpp>

#include <fmt/format.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

StandaloneTopologyHolder::StandaloneTopologyHolder(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    const Password& password,
    std::size_t database_index,
    ConnectionInfo conn
)
    : ev_thread_(sentinel_thread_control),
      redis_thread_pool_(redis_thread_pool),
      password_(std::move(password)),
      database_index_(database_index),
      conn_to_create_(conn),
      create_node_watch_(ev_thread_, [this] {
          // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
          CreateNode();
          create_node_watch_.Start();
      }) {
    LOG_DEBUG() << "Created StandaloneTopologyHolder with " << conn.host << ":" << conn.port;
}

StandaloneTopologyHolder::~StandaloneTopologyHolder() { Stop(); }

void StandaloneTopologyHolder::Init() {}

void StandaloneTopologyHolder::Start() {
    create_node_watch_.Start();
    create_node_watch_.Send();
}

void StandaloneTopologyHolder::Stop() {
    // prevent concurrent CreateNode() calls
    create_node_watch_.Stop();

    signal_node_state_change_.disconnect_all_slots();
    signal_topology_changed_.disconnect_all_slots();

    node_.Cleanup();
    topology_.Cleanup();
}

bool StandaloneTopologyHolder::WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) {
    LOG_DEBUG() << "WaitReadyOnce in mode " << ToString(mode);
    std::unique_lock lock{mutex_};
    return cv_.WaitUntil(lock, deadline, [this, mode]() {
        if (!is_nodes_received_) return false;
        auto ptr = topology_.Read();
        return ptr->IsReady(mode);
    });
}

rcu::ReadablePtr<ClusterTopology, rcu::BlockingRcuTraits> StandaloneTopologyHolder::GetTopology() const {
    return topology_.Read();
}

void StandaloneTopologyHolder::SendUpdateClusterTopology() {
    LOG_WARNING() << "SendUpdateClusterTopology is not applicable for standalone";
}

std::shared_ptr<Redis> StandaloneTopologyHolder::GetRedisInstance(const HostPort& host_port) const {
    auto node = node_.Read();
    if (node->has_value() && node->value().host_port == host_port) {
        return std::const_pointer_cast<Redis>(node->value().node->Get());
    }

    return {};
}

void StandaloneTopologyHolder::GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const {
    stats.internal.is_autotoplogy = false;
    stats.internal.cluster_topology_checks = utils::statistics::Rate{0};
    stats.internal.cluster_topology_updates =
        utils::statistics::Rate{current_topology_version_.load(std::memory_order_relaxed)};

    auto topology = GetTopology();
    topology->GetStatistics(settings, stats);
}

void StandaloneTopologyHolder::SetCommandsBufferingSettings(CommandsBufferingSettings settings) {
    {
        auto settings_ptr = commands_buffering_settings_.Lock();
        if (*settings_ptr == settings) {
            return;
        }
        *settings_ptr = settings;
    }
    auto node = node_.Read();
    if (node->has_value()) {
        node->value().node->SetCommandsBufferingSettings(settings);
    }
}

void StandaloneTopologyHolder::SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) {
    {
        auto settings_ptr = monitoring_settings_.Lock();
        *settings_ptr = settings;
    }
    auto node = node_.Read();
    if (node->has_value()) {
        node->value().node->SetReplicationMonitoringSettings(settings);
    }
}

void StandaloneTopologyHolder::SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) {
    {
        auto settings_ptr = retry_budget_settings_.Lock();
        *settings_ptr = settings;
    }
    auto node = node_.Read();
    if (node->has_value()) {
        node->value().node->SetRetryBudgetSettings(settings);
    }
}

void StandaloneTopologyHolder::SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) {
    if (info_array.size() != 1) {
        throw std::runtime_error("Single connection configuration is supported only");
    }

    auto& new_conn = info_array.front();
    LOG_DEBUG() << "Update connection info to " << new_conn.Fulltext();

    {
        const std::lock_guard<std::mutex> lock(mutex_);
        conn_to_create_ = new_conn;
        is_nodes_received_.store(false);
    }
    create_node_watch_.Send();
}

boost::signals2::signal<void(TopologyHolderBase::HostPort, Redis::State)>&
StandaloneTopologyHolder::GetSignalNodeStateChanged() {
    return signal_node_state_change_;
}

boost::signals2::signal<void(size_t)>& StandaloneTopologyHolder::GetSignalTopologyChanged() {
    return signal_topology_changed_;
}

std::shared_ptr<RedisConnectionHolder> StandaloneTopologyHolder::CreateRedisInstance(const ConnectionInfoInt& info) {
    const auto buffering_settings_ptr = commands_buffering_settings_.Lock();
    const auto replication_monitoring_settings_ptr = monitoring_settings_.Lock();
    const auto retry_budget_settings_ptr = retry_budget_settings_.Lock();
    LOG_DEBUG() << "Create new redis instance " << info.Fulltext();
    return RedisConnectionHolder::Create(
        ev_thread_,
        redis_thread_pool_,
        info.HostPort().first,
        info.HostPort().second,
        GetPassword(),
        database_index_,
        buffering_settings_ptr->value_or(CommandsBufferingSettings{}),
        *replication_monitoring_settings_ptr,
        *retry_budget_settings_ptr,
        redis::RedisCreationSettings{info.GetConnectionSecurity(), false}
    );
}

void StandaloneTopologyHolder::CreateNode() {
    LOG_DEBUG() << "Create node started";

    {
        const std::lock_guard lock{mutex_};
        // one shard
        ClusterShardHostInfos shard_infos{// only master, no slaves
                                          ClusterShardHostInfo{conn_to_create_, {}, {}}};

        if (auto topology_ptr = topology_.Read(); topology_ptr->HasSameInfos(shard_infos)) {
            LOG_INFO() << "Current topology has the same shard";
            is_nodes_received_.store(true);
            return;
        }

        auto& host_port = conn_to_create_.Fulltext();
        auto redis_connection = CreateRedisInstance(conn_to_create_);
        redis_connection->signal_state_change.connect([host_port, this](redis::RedisState state) {
            GetSignalNodeStateChanged()(host_port, state);
            { const std::lock_guard lock{mutex_}; }
            cv_.NotifyAll();
        });

        NodesStorage nodes;
        nodes.Insert(host_port, redis_connection);
        topology_.Emplace(
            ++current_topology_version_,
            std::chrono::steady_clock::now(),
            std::move(shard_infos),
            redis_thread_pool_,
            nodes
        );

        node_.Emplace(Node{std::move(host_port), redis_connection});
        is_nodes_received_.store(true);
    }

    signal_topology_changed_(1);
}

void StandaloneTopologyHolder::UpdatePassword(const Password& password) {
    auto lock = password_.UniqueLock();
    *lock = password;
}

Password StandaloneTopologyHolder::GetPassword() {
    const auto lock = password_.Lock();
    return *lock;
}

std::string StandaloneTopologyHolder::GetReadinessInfo() const {
    return fmt::format("Nodes config parsed: {}.", is_nodes_received_.load());
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
