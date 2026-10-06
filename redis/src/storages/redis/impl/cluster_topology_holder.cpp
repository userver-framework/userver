#include "cluster_topology_holder.hpp"

#include <ranges>
#include <utility>

#include <boost/container_hash/hash.hpp>
#include <engine/ev/thread_control.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/text.hpp>
#include "cluster_shards_query.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {
constexpr std::size_t kClusterDatabaseIndex = 0;

enum class ClusterNodesResponseStatus {
    kOk,
    kFail,
    kNonCluster,
};

struct NodeAddresses {
    bool operator==(const NodeAddresses& other) const noexcept {
        return ip == other.ip && fqdn_name == other.fqdn_name;
    }

    std::string ip;
    std::optional<std::string> fqdn_name;
    // Failed flag. Node is in FAIL state.
    // It was not reachable for multiple nodes that promoted the PFAIL state to FAIL.
    bool failed{false};
};

struct NodeAddressesHasher {
    std::size_t operator()(const NodeAddresses& addr) const noexcept {
        std::size_t result = 0;
        boost::hash_combine(result, addr.ip);
        if (addr.fqdn_name.has_value()) {
            boost::hash_combine(result, *addr.fqdn_name);
        }
        return result;
    }
};

using NodesAddressesSet = std::unordered_set<NodeAddresses, NodeAddressesHasher>;
using HostPort = std::string;

template <typename Callback>
std::shared_ptr<utils::FastScopeGuard<Callback>> MakeSharedScopeGuard(Callback cb) {
    return std::make_shared<utils::FastScopeGuard<Callback>>(std::move(cb));
}

std::shared_ptr<const std::vector<std::string>> MakeShardNames() {
    /// From suggested max count of nodes ~1000, with replicas, so got ~500 shards
    static const size_t kMaxClusterShards = 500;
    std::vector<std::string> shard_names;
    shard_names.reserve(kMaxClusterShards);
    for (size_t i = 0; i < kMaxClusterShards; ++i) {
        auto number = std::to_string(i);
        if (number.size() < 2) {
            number.insert(0, "0");
        }
        auto name = "shard" + number;
        shard_names.push_back(std::move(name));
    }
    return std::make_shared<const std::vector<std::string>>(std::move(shard_names));
}

bool CheckQuorum(size_t requests_sent, size_t responses_parsed) {
    const size_t quorum = requests_sent / 2 + 1;
    return responses_parsed >= quorum;
}

std::optional<std::string> GetHostNameFromClusterNodesLine(std::string_view line, std::string_view port) {
    auto it = line.rfind(',');
    if (it == std::string_view::npos) {
        return std::nullopt;
    }
    return std::string(line.substr(it + 1)) + ":" + std::string(port);
}

// Expected format (space-separated):
// id ip:port@cport flags master ping-sent pong-recv config-epoch link-state [slots...]
// The "flags" field (index 2) may contain multiple comma-separated flags,
// one of which can be "fail" indicating the node is in FAIL state.
bool IsNodeHasFailedFlag(const std::vector<std::string_view>& cluster_nodes_line) {
    if (cluster_nodes_line.size() < 3) {
        return false;
    }
    const std::string_view flags_view = cluster_nodes_line[2];
    for (auto word : std::views::split(flags_view, ',')) {
        std::string_view sv(&*word.begin(), std::ranges::distance(word));
        if (sv == "fail") {
            return true;
        }
    }
    return false;
}

ClusterNodesResponseStatus ParseClusterNodesResponse(const ReplyPtr& reply, NodesAddressesSet& res) {
    UASSERT(reply);
    if (reply->IsUnknownCommandError()) {
        return ClusterNodesResponseStatus::kNonCluster;
    }

    if (!reply->IsOk()) {
        return ClusterNodesResponseStatus::kFail;
    }

    if (!reply->data.IsString()) {
        return ClusterNodesResponseStatus::kFail;
    }

    const auto& host_lines = utils::text::SplitIntoStringViewVector(reply->data.GetString(), "\n");

    for (const auto& host_line : host_lines) {
        const auto& split = utils::text::SplitIntoStringViewVector(host_line, " ");
        if (split.size() < 2) {
            continue;
        }

        const auto& host_port_communication_port = split[1];
        if (host_port_communication_port == ":0@0" || host_port_communication_port == ":0") {
            continue;
        }
        const auto host_port_it = host_port_communication_port.rfind('@');
        auto host_port = host_port_communication_port.substr(0, host_port_it);

        const auto port_it = host_port.rfind(':');
        if (port_it == std::string::npos) {
            return ClusterNodesResponseStatus::kFail;
        }
        auto port = host_port.substr(port_it + 1);
        NodeAddresses addrs;
        addrs.ip = std::move(host_port);
        addrs.fqdn_name = GetHostNameFromClusterNodesLine(host_port_communication_port, port);
        addrs.failed = IsNodeHasFailedFlag(split);
        res.emplace(addrs);
    }

    return ClusterNodesResponseStatus::kOk;
}

}  // namespace

std::atomic<size_t> ClusterTopologyHolder::cluster_slots_call_counter(0);

ClusterTopologyHolder::ClusterTopologyHolder(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    std::string shard_group_name,
    Credentials credentials,
    const std::vector<std::string>& /*shards*/,
    const std::vector<ConnectionInfo>& conns,
    ConnectionSecurity connection_security,
    TopologyUpdateMethod topology_update_method,
    Hysteresis::Config hysteresis_config,
    std::chrono::seconds max_disconnect_time
)
    : ev_thread_(sentinel_thread_control),
      redis_thread_pool_(redis_thread_pool),
      shard_group_name_(std::move(shard_group_name)),
      credentials_(std::move(credentials)),
      shards_names_(MakeShardNames()),
      conns_(conns),
      topology_update_method_(topology_update_method),
      statistics_holder_(),
      update_topology_timer_(ev_thread_, [this] { UpdateClusterTopology(); }, kSentinelGetHostsCheckInterval),
      update_topology_watch_(
          ev_thread_,
          [this] {
              UpdateClusterTopology();
              update_topology_watch_.Start();
          }
      ),
      explore_nodes_watch_(
          ev_thread_,
          [this] {
              ExploreNodes();
              explore_nodes_watch_.Start();
          }
      ),
      explore_nodes_timer_(ev_thread_, [this] { ExploreNodes(); }, kSentinelGetHostsCheckInterval),
      create_nodes_watch_(
          ev_thread_,
          [this] {
              CreateNodes();
              create_nodes_watch_.Start();
          }
      ),
      delete_expired_nodes_timer_(ev_thread_, [this] { DeleteNodes(); }, kDeleteNodesCheckInterval),
      sentinels_process_creation_timer_(
          ev_thread_,
          [this] {
              if (sentinels_) {
                  sentinels_->ProcessCreation(redis_thread_pool_);
                  sentinels_->ProcessStateUpdate();
              }
          },
          kProcessCreationInterval
      ),
      sentinels_process_creation_watch_(
          ev_thread_,
          [this] {
              if (sentinels_) {
                  sentinels_->ProcessCreation(redis_thread_pool_);
              }
              sentinels_process_creation_watch_.Start();
          }
      ),
      sentinels_process_state_update_watch_(
          ev_thread_,
          [this] {
              if (sentinels_) {
                  sentinels_->ProcessStateUpdate();
              }
              sentinels_process_state_update_watch_.Start();
          }
      ),
      is_topology_received_(false),
      connection_security_(connection_security),
      hysteresis_config_(hysteresis_config),
      max_disconnect_time_(max_disconnect_time)
{
    log_extra_.Extend("shard_group_name", shard_group_name_);
    LOG_DEBUG() << log_extra_ << "Created ClusterTopologyHolder";
}

void ClusterTopologyHolder::Init() {
    const auto callback_token = GetCallbackToken();
    const constexpr bool kClusterMode = true;
    Shard::Options shard_options;
    shard_options.shard_name = "(sentinel)";
    shard_options.shard_group_name = shard_group_name_;
    shard_options.cluster_mode = kClusterMode;
    shard_options.connection_infos = conns_;
    shard_options.ready_change_callback = [this, callback_token](bool ready) {
        if (!AreCallbacksEnabled(callback_token)) {
            return;
        }
        if (ready) {
            sentinels_process_creation_watch_.Send();
            SendUpdateClusterTopology();
        }
    };

    sentinels_ = std::make_shared<Shard>(std::move(shard_options), ev_thread_);

    sentinels_->SignalInstanceStateChange().connect([this, callback_token](ServerId id, Redis::State state) {
        if (!AreCallbacksEnabled(callback_token)) {
            return;
        }
        LOG_TRACE() << log_extra_ << "Signaled server " << id.GetDescription() << " state=" << StateToString(state);
        if (state != Redis::State::kInit) {
            sentinels_process_state_update_watch_.Send();
        }
    });
    sentinels_->SignalInstanceReady().connect([this, callback_token](ServerId, bool /*readonly*/) {
        if (!AreCallbacksEnabled(callback_token)) {
            return;
        }
        if (!std::exchange(first_entry_point_connected_, true)) {
            explore_nodes_watch_.Send();
        }
    });
    sentinels_->ProcessCreation(redis_thread_pool_);
}

void ClusterTopologyHolder::Start() {
    update_topology_watch_.Start();
    update_topology_timer_.Start();
    create_nodes_watch_.Start();
    explore_nodes_watch_.Start();
    explore_nodes_timer_.Start();
    delete_expired_nodes_timer_.Start();
    sentinels_process_creation_watch_.Start();
    sentinels_process_state_update_watch_.Start();
    sentinels_process_creation_timer_.Start();
}

void ClusterTopologyHolder::Stop() {
    DisableCallbacks();
    signal_node_state_change_.disconnect_all_slots();
    signal_topology_changed_.disconnect_all_slots();

    update_topology_watch_.Stop();
    create_nodes_watch_.Stop();
    explore_nodes_watch_.Stop();

    update_topology_timer_.Stop();
    explore_nodes_timer_.Stop();
    delete_expired_nodes_timer_.Stop();
    sentinels_process_creation_timer_.Stop();

    sentinels_->Clean();
    topology_.Assign(ClusterTopology{});
    nodes_.Clear();
}

bool ClusterTopologyHolder::WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) {
    return readiness_event_.WaitUntil(deadline, [this, mode] {
        return IsReady(HealthCheckParams{mode, 0, 0});
    }) == engine::FutureStatus::kReady;
}

bool ClusterTopologyHolder::IsReady(const HealthCheckParams& params) const {
    if (!is_nodes_received_.load()) {
        return false;
    }
    if (!is_topology_received_.load()) {
        return false;
    }

    auto ptr = topology_.Read();
    return ptr->IsReady(params);
}

rcu::ReadablePtr<ClusterTopology, rcu::ExclusiveRcuTraits> ClusterTopologyHolder::GetTopology() const {
    return topology_.Read();
}

void ClusterTopologyHolder::SendUpdateClusterTopology() { update_topology_watch_.Send(); }

std::shared_ptr<Redis> ClusterTopologyHolder::GetRedisInstance(const HostPort& host_port) const {
    const auto connection = nodes_.Get(host_port);
    if (connection) {
        return std::const_pointer_cast<Redis>(connection->Get());
    }
    if (auto ip = ip_by_fqdn_.Get(host_port); ip) {
        const auto connection = nodes_.Get(*ip);
        if (connection) {
            return std::const_pointer_cast<Redis>(connection->Get());
        }
    }

    return {};
}

void ClusterTopologyHolder::SetCommandsBufferingSettings(CommandsBufferingSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    if (commands_buffering_settings_ == settings) {
        return;
    }
    commands_buffering_settings_ = settings;
    for (const auto& node : nodes_) {
        node.second->SetCommandsBufferingSettings(settings);
    }
}

void ClusterTopologyHolder::SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) {
    UASSERT(ev_thread_.IsInEvThread());
    monitoring_settings_ = settings;
    for (const auto& node : nodes_) {
        node.second->SetReplicationMonitoringSettings(settings);
    }
}

void ClusterTopologyHolder::SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) {
    UASSERT(ev_thread_.IsInEvThread());
    retry_budget_settings_ = settings;
    for (const auto& node : nodes_) {
        node.second->SetRetryBudgetSettings(settings);
    }
}

void ClusterTopologyHolder::SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) {
    sentinels_->SetConnectionInfo(info_array);
}

boost::signals2::signal<void(HostPort, Redis::State)>& ClusterTopologyHolder::GetSignalNodeStateChanged() {
    return signal_node_state_change_;
}

boost::signals2::signal<void(size_t)>& ClusterTopologyHolder::GetSignalTopologyChanged() {
    return signal_topology_changed_;
}

void ClusterTopologyHolder::UpdateCredentials(const Credentials& credentials) {
    UASSERT(ev_thread_.IsInEvThread());
    credentials_ = credentials;
}

Credentials ClusterTopologyHolder::GetCredentials() {
    UASSERT(ev_thread_.IsInEvThread());
    return credentials_;
}

std::string ClusterTopologyHolder::GetReadinessInfo() const {
    return fmt::format(
        "Nodes received: {}; topology received: {}.",
        is_nodes_received_.load(),
        is_topology_received_.load()
    );
}

void ClusterTopologyHolder::ExploreNodes() {
    /// Call cluster nodes, parse, prepare list of new hosts to create
    if (!sentinels_) {
        return;
    }

    const auto callback_token = GetCallbackToken();
    const auto log_extra = log_extra_;
    const auto cmd = PrepareCommand(
        {"CLUSTER", "NODES"},
        [this,
         callback_token,
         thread_control = ev_thread_,
         log_extra](const CommandPtr& /*cmd*/, ReplyPtr reply) mutable {
            NodesAddressesSet host_ports;
            if (ParseClusterNodesResponse(reply, host_ports) != ClusterNodesResponseStatus::kOk) {
                LOG_WARNING() << log_extra << "Failed to parse CLUSTER NODES response";
                return;
            }

            thread_control
                .RunInEvLoopAsync([this, callback_token, host_ports = std::move(host_ports)]() mutable noexcept {
                    if (!AreCallbacksEnabled(callback_token)) {
                        return;
                    }
                    try {
                        std::unordered_set<HostPort> host_ports_to_create;
                        for (const auto& host_port : host_ports) {
                            const auto node = nodes_.Get(host_port.ip);
                            if (!node) {
                                host_ports_to_create.insert(host_port.ip);
                            } else {
                                node->AccountFail(host_port.failed);
                            }
                        }
                        if (!host_ports.empty()) {
                            for (const auto& address : host_ports) {
                                if (!address.fqdn_name.has_value()) {
                                    continue;
                                }
                                const auto ptr = ip_by_fqdn_.Get(*address.fqdn_name);
                                if (!ptr || *ptr != address.ip) {
                                    ip_by_fqdn_
                                        .InsertOrAssign(*address.fqdn_name, std::make_shared<std::string>(address.ip));
                                }
                            }

                            std::unordered_set<HostPort> ips;
                            for (auto& address : host_ports) {
                                ips.insert(std::move(address.ip));
                            }
                            actual_nodes_.merge(std::move(ips));
                        }

                        if (!host_ports_to_create.empty()) {
                            std::swap(nodes_to_create_, host_ports_to_create);
                            create_nodes_watch_.Send();
                        }
                    } catch (const std::exception& ex) {
                        LOG_ERROR() << log_extra_ << "Failed to apply CLUSTER NODES response: " << ex;
                    }
                });
        }
    );
    sentinels_->AsyncCommand(cmd);
}

void ClusterTopologyHolder::CreateNodes() {
    UASSERT(ev_thread_.IsInEvThread());
    std::unordered_set<HostPort> host_ports_to_create;
    std::swap(nodes_to_create_, host_ports_to_create);

    for (auto&& host_port : host_ports_to_create) {
        auto instance = CreateRedisInstance(host_port);
        const auto callback_token = GetCallbackToken();
        instance->signal_state_change.connect([host_port, this, callback_token](redis::RedisState state) {
            if (!AreCallbacksEnabled(callback_token)) {
                return;
            }
            GetSignalNodeStateChanged()(host_port, state);
            readiness_event_.Send();
        });
        nodes_.Insert(std::move(host_port), std::move(instance));
    }

    if (!is_nodes_received_.exchange(true)) {
        SendUpdateClusterTopology();
    }
}

void ClusterTopologyHolder::DeleteNodes() {
    UASSERT(ev_thread_.IsInEvThread());
    std::unordered_set<HostPort> actual_nodes;
    std::swap(actual_nodes_, actual_nodes);
    const auto now = utils::datetime::SteadyCoarseClock::now();
    for (auto& node : actual_nodes) {
        nodes_last_seen_time_[node] = now;
    }
    utils::EraseIf(nodes_last_seen_time_, [this, now](const auto& node_time) {
        const auto& [node, time] = node_time;
        if (now - time >= kDeleteNodeInterval) {
            nodes_.Erase(node);
            return true;
        }
        return false;
    });
}

std::shared_ptr<RedisConnectionHolder> ClusterTopologyHolder::CreateRedisInstance(const std::string& host_port) {
    UASSERT(ev_thread_.IsInEvThread());
    const auto port_it = host_port.rfind(':');
    UINVARIANT(port_it != std::string::npos, "port must be delimited by ':'");
    const auto port_str = host_port.substr(port_it + 1);
    const auto port = std::stoi(port_str);
    const auto host = host_port.substr(0, port_it);
    LOG_DEBUG() << log_extra_ << "Create new redis instance " << host_port;
    auto creation_settings = RedisConnectionHolder::makeClusterNodeRedisCreationSettings();
    creation_settings.connection_security = connection_security_;
    return RedisConnectionHolder::Create(
        ev_thread_,
        redis_thread_pool_,
        shard_group_name_,
        host,
        port,
        GetCredentials(),
        kClusterDatabaseIndex,
        commands_buffering_settings_.value_or(CommandsBufferingSettings{}),
        monitoring_settings_,
        retry_budget_settings_,
        statistics_holder_.MakeInstanceStats(),
        creation_settings,
        hysteresis_config_,
        max_disconnect_time_
    );
}

void ClusterTopologyHolder::UpdateClusterTopology() {
    UASSERT(ev_thread_.IsInEvThread());
    if (!is_nodes_received_) {
        LOG_DEBUG() << log_extra_ << "Skip updating cluster topology: no nodes yet";
        return;
    };
    if (update_cluster_slots_flag_->exchange(true)) {
        return;
    }
    const auto callback_token = GetCallbackToken();
    auto reset_update_cluster_slots = MakeSharedScopeGuard([flag = update_cluster_slots_flag_]() noexcept {
        flag->store(false);
    });
    /// Update sentinel
    sentinels_->ProcessCreation(redis_thread_pool_);

    auto callback =
        [this,
         callback_token,
         reset{std::move(reset_update_cluster_slots)
         }](ClusterShardHostInfos shard_infos, size_t requests_sent, size_t responses_parsed, bool is_non_cluster_error
        ) {
            if (!AreCallbacksEnabled(callback_token)) {
                return;
            }
            LOG_DEBUG()
                << log_extra_ << "Parsing response from cluster shards: shard_infos.size(): " << shard_infos.size()
                << ", requests_sent=" << requests_sent << ", responses_parsed=" << responses_parsed;
            const auto deferred = utils::FastScopeGuard([&]() noexcept { ++cluster_slots_call_counter; });
            if (is_non_cluster_error) {
                LOG_ERROR()
                    << log_extra_ << "Redis must be in cluster mode; shard_infos.size(): " << shard_infos.size();
                return;
            }
            if (shard_infos.empty()) {
                LOG_WARNING() << log_extra_ << "Received empty topology";
                return;
            }

            if (!CheckQuorum(requests_sent, responses_parsed)) {
                LOG_WARNING()
                    << log_extra_ << "Too many 'cluster slots' requests failed: requests_sent=" << requests_sent
                    << " responses_parsed=" << responses_parsed;
                return;
            }

            {
                auto temp_read_ptr = topology_.Read();
                if (temp_read_ptr->HasSameInfos(shard_infos)) {
                    /// Nothing new here so do nothing
                    return;
                }

                if (temp_read_ptr->GetShardInfos().size() != shard_infos.size()) {
                    LOG_WARNING()
                        << log_extra_ << "Significant change of Redis cluster topology. From "
                        << temp_read_ptr->GetShardInfos().size() << " shards count to " << shard_infos.size();
                }
            }

            auto topology = ClusterTopology(
                ++current_topology_version_,
                std::chrono::steady_clock::now(),
                std::move(shard_infos),
                redis_thread_pool_,
                nodes_
            );

            LOG_INFO() << log_extra_ << [&infos_list = topology.GetShardInfos()](auto& out) {
                out << "New Redis cluster topology: ";
                bool first_info_record = true;
                for (const auto& info : infos_list) {
                    if (!first_info_record) {
                        out << "; ";
                    }

                    const auto& [host, port] = info.master.HostPort();
                    out << "[" << host << "]:" << port << " master with replicas ";
                    bool first_replica_record = true;
                    for (const auto& replica_info : info.slaves) {
                        const auto& [host, port] = replica_info.HostPort();
                        if (!first_replica_record) {
                            out << ", ";
                        }

                        out << "[" << host << "]:" << port;
                        first_replica_record = false;
                    }

                    out << " for slots " << info.slot_intervals;
                    first_info_record = false;
                }
            };

            try {
                const auto new_shards_count = topology.GetShardsCount();
                topology_.Assign(std::move(topology));
                signal_topology_changed_(new_shards_count);
            } catch (const rcu::MissingKeyException& e) {
                LOG_WARNING() << log_extra_ << "Failed to update cluster topology: " << e;
                return;
            }
            is_topology_received_ = true;
            readiness_event_.Send();

            LOG_DEBUG() << log_extra_ << "Cluster topology updated to version" << current_topology_version_.load();
        };

    switch (topology_update_method_) {
        case TopologyUpdateMethod::kClusterShards:
            GetClusterShardsContext::ProcessRequest(
                ev_thread_,
                shards_names_,
                GetClusterShardsRequest(*sentinels_, GetCredentials(), shard_group_name_),
                std::move(callback)
            );
            break;
        case TopologyUpdateMethod::kClusterSlots:
            GetClusterSlotsContext::ProcessRequest(
                ev_thread_,
                shards_names_,
                GetClusterSlotsRequest(*sentinels_, GetCredentials(), shard_group_name_),
                std::move(callback)
            );
            break;
    }
}

void ClusterTopologyHolder::GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const {
    if (sentinels_) {
        stats.sentinel.emplace(ShardStatistics(settings));
        sentinels_->GetStatistics(true, *stats.sentinel);
    }
    stats.internal.is_autotopology = true;
    stats.internal.cluster_topology_checks = utils::statistics::Rate{
        cluster_slots_call_counter.load(std::memory_order_relaxed)
    };
    stats.internal.cluster_topology_updates = utils::statistics::Rate{
        current_topology_version_.load(std::memory_order_relaxed)
    };

    auto topology = GetTopology();
    statistics_holder_.GetStatistics(stats, *topology);
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
