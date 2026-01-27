#include "cluster_topology_holder.hpp"

#include <boost/container_hash/hash.hpp>
#include <engine/ev/thread_control.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/text.hpp>

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
        const auto& splitted = utils::text::SplitIntoStringViewVector(host_line, " ");
        if (splitted.size() < 2) {
            continue;
        }

        const auto& host_port_communication_port = splitted[1];
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
    Password password,
    const std::vector<std::string>& /*shards*/,
    const std::vector<ConnectionInfo>& conns,
    ConnectionSecurity connection_security
)
    : ev_thread_(sentinel_thread_control),
      redis_thread_pool_(redis_thread_pool),
      shard_group_name_(std::move(shard_group_name)),
      password_(std::move(password)),
      shards_names_(MakeShardNames()),
      conns_(conns),
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
      update_cluster_slots_flag_(false),
      connection_security_(connection_security)
{
    log_extra_.Extend("shard_group_name", shard_group_name_);
    LOG_DEBUG() << log_extra_ << "Created ClusterTopologyHolder";
}

void ClusterTopologyHolder::Init() {
    const constexpr bool kClusterMode = true;
    Shard::Options shard_options;
    shard_options.shard_name = "(sentinel)";
    shard_options.shard_group_name = shard_group_name_;
    shard_options.cluster_mode = kClusterMode;
    shard_options.connection_infos = conns_;
    shard_options.ready_change_callback = [this](bool ready) {
        if (ready) {
            sentinels_process_creation_watch_.Send();
            SendUpdateClusterTopology();
        }
    };

    sentinels_ = std::make_shared<Shard>(std::move(shard_options));

    sentinels_->SignalInstanceStateChange().connect([this](ServerId id, Redis::State state) {
        LOG_TRACE() << log_extra_ << "Signaled server " << id.GetDescription() << " state=" << StateToString(state);
        if (state != Redis::State::kInit) {
            sentinels_process_state_update_watch_.Send();
        }
    });
    sentinels_->SignalInstanceReady().connect([this](ServerId, bool /*readonly*/) {
        if (!first_entry_point_connected_.exchange(true)) {
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
    signal_node_state_change_.disconnect_all_slots();
    signal_topology_changed_.disconnect_all_slots();

    ev_thread_.RunInEvLoopBlocking([this] {
        update_topology_watch_.Stop();
        create_nodes_watch_.Stop();
        explore_nodes_watch_.Stop();

        update_topology_timer_.Stop();
        explore_nodes_timer_.Stop();
        delete_expired_nodes_timer_.Stop();
        sentinels_process_creation_timer_.Stop();
    });

    sentinels_->Clean();
    topology_.Cleanup();
    nodes_.Clear();
}

bool ClusterTopologyHolder::WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) {
    std::unique_lock lock{mutex_};
    return cv_.WaitUntil(lock, deadline, [this, mode]() {
        if (!IsInitialized()) {
            return false;
        }
        auto ptr = topology_.Read();
        return ptr->IsReady(mode);
    });
}

rcu::ReadablePtr<ClusterTopology, rcu::BlockingRcuTraits> ClusterTopologyHolder::GetTopology() const {
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
    {
        auto settings_ptr = commands_buffering_settings_.Lock();
        if (*settings_ptr == settings) {
            return;
        }
        *settings_ptr = settings;
    }
    for (const auto& node : nodes_) {
        node.second->SetCommandsBufferingSettings(settings);
    }
}

void ClusterTopologyHolder::SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) {
    {
        auto settings_ptr = monitoring_settings_.Lock();
        *settings_ptr = settings;
    }
    for (const auto& node : nodes_) {
        node.second->SetReplicationMonitoringSettings(settings);
    }
}

void ClusterTopologyHolder::SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) {
    {
        auto settings_ptr = retry_budget_settings_.Lock();
        *settings_ptr = settings;
    }
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

void ClusterTopologyHolder::UpdatePassword(const Password& password) {
    auto lock = password_.UniqueLock();
    *lock = password;
}

Password ClusterTopologyHolder::GetPassword() {
    const auto lock = password_.Lock();
    return *lock;
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

    const auto cmd = PrepareCommand({"CLUSTER", "NODES"}, [this](const CommandPtr& /*cmd*/, ReplyPtr reply) {
        NodesAddressesSet host_ports;
        std::unordered_set<HostPort> host_ports_to_create;

        if (ParseClusterNodesResponse(reply, host_ports) != ClusterNodesResponseStatus::kOk) {
            LOG_WARNING() << log_extra_ << "Failed to parse CLUSTER NODES response";
            return;
        }

        for (const auto& host_port : host_ports) {
            if (!nodes_.Get(host_port.ip)) {
                host_ports_to_create.insert(host_port.ip);
            }
        }
        if (!host_ports.empty()) {
            for (const auto& [ip, fqdn] : host_ports) {
                if (!fqdn.has_value()) {
                    continue;
                }
                auto ptr = ip_by_fqdn_.Get(*fqdn);
                if (!ptr || *ptr != ip) {
                    ip_by_fqdn_.InsertOrAssign(*fqdn, std::make_shared<std::string>(ip));
                }
            }

            std::unordered_set<HostPort> ips;
            {
                for (auto& addr : host_ports) {
                    ips.insert(std::move(addr.ip));
                }
                auto ptr = actual_nodes_.Lock();
                ptr->merge(std::move(ips));
            }
        }

        if (!host_ports_to_create.empty()) {
            {
                auto ptr = nodes_to_create_.Lock();
                std::swap(*ptr, host_ports_to_create);
            }
            create_nodes_watch_.Send();
        }
    });
    sentinels_->AsyncCommand(cmd);
}

void ClusterTopologyHolder::CreateNodes() {
    std::unordered_set<HostPort> host_ports_to_create;
    {
        auto ptr = nodes_to_create_.Lock();
        std::swap(*ptr, host_ports_to_create);
    }

    for (auto&& host_port : host_ports_to_create) {
        auto instance = CreateRedisInstance(host_port);
        instance->signal_state_change.connect([host_port, this](redis::RedisState state) {
            GetSignalNodeStateChanged()(host_port, state);
            {
                const std::lock_guard lock{mutex_};
            }  // do not lose the notify
            cv_.NotifyAll();
        });
        nodes_.Insert(std::move(host_port), std::move(instance));
    }

    if (!is_nodes_received_.exchange(true)) {
        SendUpdateClusterTopology();
    }
}

void ClusterTopologyHolder::DeleteNodes() {
    std::unordered_set<HostPort> actual_nodes;
    {
        auto ptr = actual_nodes_.Lock();
        std::swap(*ptr, actual_nodes);
    }
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
    const auto port_it = host_port.rfind(':');
    UINVARIANT(port_it != std::string::npos, "port must be delimited by ':'");
    const auto port_str = host_port.substr(port_it + 1);
    const auto port = std::stoi(port_str);
    const auto host = host_port.substr(0, port_it);
    const auto buffering_settings_ptr = commands_buffering_settings_.Lock();
    const auto replication_monitoring_settings_ptr = monitoring_settings_.Lock();
    const auto retry_budget_settings_ptr = retry_budget_settings_.Lock();
    LOG_DEBUG() << log_extra_ << "Create new redis instance " << host_port;
    auto creation_settings = RedisConnectionHolder::makeClusterNodeRedisCreationSettings();
    creation_settings.connection_security = connection_security_;
    return RedisConnectionHolder::Create(
        ev_thread_,
        redis_thread_pool_,
        shard_group_name_,
        host,
        port,
        GetPassword(),
        kClusterDatabaseIndex,
        buffering_settings_ptr->value_or(CommandsBufferingSettings{}),
        *replication_monitoring_settings_ptr,
        *retry_budget_settings_ptr,
        creation_settings
    );
}

void ClusterTopologyHolder::UpdateClusterTopology() {
    if (!is_nodes_received_) {
        LOG_DEBUG() << log_extra_ << "Skip updating cluster topology: no nodes yet";
        return;
    };
    if (update_cluster_slots_flag_.exchange(true)) {
        return;
    }
    auto reset_update_cluster_slots = MakeSharedScopeGuard([&]() noexcept { update_cluster_slots_flag_ = false; });
    /// Update sentinel
    sentinels_->ProcessCreation(redis_thread_pool_);

    /// Update controlled topology. Go to CLUSTER SLOTS
    /// ...
    ProcessGetClusterHostsRequest(
        shards_names_,
        GetClusterHostsRequest(*sentinels_, GetPassword(), shard_group_name_),
        [this,
         reset{std::move(reset_update_cluster_slots)
         }](ClusterShardHostInfos shard_infos, size_t requests_sent, size_t responses_parsed, bool is_non_cluster_error
        ) {
            LOG_DEBUG()
                << log_extra_ << "Parsing response from cluster slots: shard_infos.size(): " << shard_infos.size()
                << ", requests_sent=" << requests_sent << ", responses_parsed=" << responses_parsed;
            const auto deferred = utils::FastScopeGuard([&]() noexcept { ++cluster_slots_call_counter; });
            if (is_non_cluster_error) {
                LOG_DEBUG() << log_extra_ << "Non cluster error: shard_infos.size(): " << shard_infos.size();
                throw std::runtime_error("Redis must be in cluster mode");
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

            /// Run in ev_thread because topology_.Assign can free some old
            /// topologies with their related redis connections, and these
            /// connections must be freed on "sentinel" thread.
            ev_thread_.RunInEvLoopAsync([this, topology{std::move(topology)}]() mutable {
                try {
                    const auto new_shards_count = topology.GetShardsCount();
                    topology_.Assign(std::move(topology));
                    signal_topology_changed_(new_shards_count);
                } catch (const rcu::MissingKeyException& e) {
                    LOG_WARNING() << log_extra_ << "Failed to update cluster topology: " << e;
                    return;
                }
                is_topology_received_ = true;
                {
                    const std::lock_guard lock{mutex_};
                }  // do not lose the notify
                cv_.NotifyAll();

                LOG_DEBUG() << log_extra_ << "Cluster topology updated to version" << current_topology_version_.load();
            });
        }
    );
}

void ClusterTopologyHolder::GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const {
    if (sentinels_) {
        stats.sentinel.emplace(ShardStatistics(settings));
        sentinels_->GetStatistics(true, settings, *stats.sentinel);
    }
    stats.internal.is_autotopology = true;
    stats.internal.cluster_topology_checks = utils::statistics::Rate{
        cluster_slots_call_counter.load(std::memory_order_relaxed)
    };
    stats.internal.cluster_topology_updates = utils::statistics::Rate{
        current_topology_version_.load(std::memory_order_relaxed)
    };

    auto topology = GetTopology();
    topology->GetStatistics(settings, stats);
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
