#include <storages/redis/impl/standalone_topology_holder.hpp>

#include <fmt/format.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

StandaloneTopologyHolder::StandaloneTopologyHolder(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    const std::string& shard_group_name,
    const Credentials& credentials,
    std::size_t database_index,
    ConnectionInfo conn
)
    : ev_thread_(sentinel_thread_control),
      redis_thread_pool_(redis_thread_pool),
      shard_group_name_(shard_group_name),
      credentials_(credentials),
      database_index_(database_index),
      conn_to_create_(conn),
      create_node_watch_(
          ev_thread_,
          [this] {
              // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
              CreateNode();
              create_node_watch_.Start();
          }
      )
{
    LOG_DEBUG() << "Created StandaloneTopologyHolder with " << conn.host << ":" << conn.port;
}

StandaloneTopologyHolder::~StandaloneTopologyHolder() { Stop(); }

void StandaloneTopologyHolder::Init() {}

void StandaloneTopologyHolder::Start() {
    create_node_watch_.Start();
    create_node_watch_.Send();
}

void StandaloneTopologyHolder::Stop() {
    DisableCallbacks();
    // prevent concurrent CreateNode() calls
    create_node_watch_.Stop();

    signal_node_state_change_.disconnect_all_slots();
    signal_topology_changed_.disconnect_all_slots();

    topology_.Assign(ClusterTopology{});
    node_.Assign(std::nullopt);
}

bool StandaloneTopologyHolder::WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) {
    LOG_DEBUG() << "WaitReadyOnce in mode " << ToString(mode);
    return readiness_event_.WaitUntil(deadline, [this, mode] {
        return IsReady(HealthCheckParams{mode, 0, 0});
    }) == engine::FutureStatus::kReady;
}

bool StandaloneTopologyHolder::IsReady(const HealthCheckParams& params) const {
    if (!is_nodes_received_) {
        return false;
    }
    auto ptr = topology_.Read();
    return ptr->IsReady(params);
}

rcu::ReadablePtr<ClusterTopology, rcu::ExclusiveRcuTraits> StandaloneTopologyHolder::GetTopology() const {
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

void StandaloneTopologyHolder::GetStatistics(SentinelStatistics& stats, const MetricsSettings&) const {
    stats.internal.is_autotopology = false;
    stats.internal.cluster_topology_checks = utils::statistics::Rate{0};
    stats.internal.cluster_topology_updates = utils::statistics::Rate{
        current_topology_version_.load(std::memory_order_relaxed)
    };

    auto topology = GetTopology();
    statistics_holder_.GetStatistics(stats, *topology);
}

void StandaloneTopologyHolder::SetCommandsBufferingSettings(CommandsBufferingSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    if (commands_buffering_settings_ == settings) {
        return;
    }
    commands_buffering_settings_ = settings;
    auto node = node_.Read();
    if (node->has_value()) {
        node->value().node->SetCommandsBufferingSettings(settings);
    }
}

void StandaloneTopologyHolder::SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    monitoring_settings_ = settings;
    auto node = node_.Read();
    if (node->has_value()) {
        node->value().node->SetReplicationMonitoringSettings(settings);
    }
}

void StandaloneTopologyHolder::SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) {
    UASSERT(ev_thread_.IsInEvThread());
    retry_budget_settings_ = settings;
    auto node = node_.Read();
    if (node->has_value()) {
        node->value().node->SetRetryBudgetSettings(settings);
    }
}

void StandaloneTopologyHolder::SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) {
    UASSERT(ev_thread_.IsInEvThread());
    if (info_array.size() != 1) {
        throw std::runtime_error("Single connection configuration is supported only");
    }

    auto& new_conn = info_array.front();
    LOG_DEBUG() << "Update connection info to " << new_conn.Fulltext();

    conn_to_create_ = new_conn;
    is_nodes_received_.store(false);
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
    UASSERT(ev_thread_.IsInEvThread());
    LOG_DEBUG() << "Create new redis instance " << info.Fulltext();
    return RedisConnectionHolder::Create(
        ev_thread_,
        redis_thread_pool_,
        shard_group_name_,
        info.HostPort().first,
        info.HostPort().second,
        GetCredentials(),
        database_index_,
        commands_buffering_settings_.value_or(CommandsBufferingSettings{}),
        monitoring_settings_,
        retry_budget_settings_,
        statistics_holder_.MakeInstanceStats(),
        redis::RedisCreationSettings{info.GetConnectionSecurity(), false}
    );
}

void StandaloneTopologyHolder::CreateNode() {
    UASSERT(ev_thread_.IsInEvThread());
    LOG_DEBUG() << "Create node started";

    // one shard
    ClusterShardHostInfos shard_infos{
        // only master, no slaves
        ClusterShardHostInfo{conn_to_create_, {}, {}}
    };

    if (auto topology_ptr = topology_.Read(); topology_ptr->HasSameInfos(shard_infos)) {
        LOG_INFO() << "Current topology has the same shard";
        is_nodes_received_.store(true);
        readiness_event_.Send();
        return;
    }

    auto host_port = conn_to_create_.Fulltext();
    auto redis_connection = CreateRedisInstance(conn_to_create_);
    const auto callback_token = GetCallbackToken();
    redis_connection->signal_state_change.connect([host_port, this, callback_token](redis::RedisState state) {
        if (!AreCallbacksEnabled(callback_token)) {
            return;
        }
        GetSignalNodeStateChanged()(host_port, state);
        readiness_event_.Send();
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
    readiness_event_.Send();

    signal_topology_changed_(1);
}

void StandaloneTopologyHolder::UpdateCredentials(const Credentials& credentials) {
    UASSERT(ev_thread_.IsInEvThread());
    credentials_ = credentials;
}

Credentials StandaloneTopologyHolder::GetCredentials() {
    UASSERT(ev_thread_.IsInEvThread());
    return credentials_;
}

std::string StandaloneTopologyHolder::GetReadinessInfo() const {
    return fmt::format("Nodes config parsed: {}.", is_nodes_received_.load());
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
