#include "sentinel_topology_holder.hpp"

#include <atomic>

#include <storages/redis/impl/cluster_topology.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/redis_stats.hpp>
#include <storages/redis/impl/sentinel_query.hpp>
#include <storages/redis/impl/shard.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>
#include <userver/utils/retry_budget.hpp>
#include <userver/utils/scope_guard.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

// https://github.com/boostorg/signals2/issues/59
// NOLINTBEGIN(clang-analyzer-cplusplus.NewDelete)

namespace storages::redis::impl {
namespace {
constexpr auto kProcessCreationInterval = std::chrono::seconds(3);

bool CheckQuorum(size_t requests_sent, size_t responses_parsed) {
    const size_t quorum = requests_sent / 2 + 1;
    return responses_parsed >= quorum;
}

template <typename Callback>
std::shared_ptr<utils::ScopeGuard> MakeSharedScopeGuard(Callback cb) {
    return std::make_shared<utils::ScopeGuard>(std::move(cb));
}

}  // namespace

SentinelTopologyHolder::SentinelTopologyHolder(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    const std::string& shard_group_name,
    const Password& password,
    std::size_t database_index,
    const std::vector<std::string>& shard_names,
    const std::vector<ConnectionInfo>& conns,
    ConnectionSecurity connection_security
)
    : ev_thread_(sentinel_thread_control),
      redis_thread_pool_(redis_thread_pool),
      shard_group_name_(shard_group_name),
      password_(password),
      database_index_(database_index),
      name_by_shard_(shard_names),
      conns_(conns),
      update_topology_timer_(ev_thread_, [this] { UpdateClusterTopology(); }, kSentinelGetHostsCheckInterval),
      update_topology_watch_(
          ev_thread_,
          [this] {
              UpdateClusterTopology();
              update_topology_watch_.Start();
          }
      ),
      create_instances_and_update_topology_watch_(
          ev_thread_,
          [this] {
              CreateInstancesAndUpdateTopology();
              create_instances_and_update_topology_watch_.Start();
          }
      ),
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
      connection_security_(connection_security)
{
    log_extra_.Extend("shard_group_name", shard_group_name_);
    LOG_DEBUG() << log_extra_ << "Created ClusterTopologyHolder";
    size_t shard_count = 0;
    for (const auto& name : shard_names) {
        shard_by_name_[name] = shard_count++;
    }
}

SentinelTopologyHolder::~SentinelTopologyHolder() = default;

void SentinelTopologyHolder::Init() {
    const constexpr bool kClusterMode = true;
    Shard::Options shard_options;
    shard_options.shard_name = "(sentinel)";
    shard_options.shard_group_name = shard_group_name_;
    shard_options.cluster_mode = kClusterMode;
    shard_options.connection_infos = conns_.ReadCopy();
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
            update_topology_watch_.Send();
        }
    });
    sentinels_->ProcessCreation(redis_thread_pool_);
}

void SentinelTopologyHolder::Start() {
    update_topology_watch_.Start();
    update_topology_timer_.Start();
    create_instances_and_update_topology_watch_.Start();
    sentinels_process_creation_watch_.Start();
    sentinels_process_state_update_watch_.Start();
    sentinels_process_creation_timer_.Start();
}

void SentinelTopologyHolder::Stop() {
    signal_node_state_change_.disconnect_all_slots();
    signal_topology_changed_.disconnect_all_slots();

    ev_thread_.RunInEvLoopBlocking([this] {
        update_topology_watch_.Stop();
        create_instances_and_update_topology_watch_.Stop();

        update_topology_timer_.Stop();
        sentinels_process_creation_timer_.Stop();
    });

    sentinels_->Clean();
    topology_.Cleanup();
    nodes_.Clear();
}

bool SentinelTopologyHolder::WaitReadyOnce(engine::Deadline deadline, WaitConnectedMode mode) {
    std::unique_lock lock{mutex_};
    return cv_.WaitUntil(lock, deadline, [this, mode]() {
        if (!IsInitialized()) {
            return false;
        }
        auto ptr = topology_.Read();
        return ptr->IsReady(mode);
    });
}

rcu::ReadablePtr<ClusterTopology, rcu::BlockingRcuTraits> SentinelTopologyHolder::GetTopology() const {
    return topology_.Read();
}

void SentinelTopologyHolder::SendUpdateClusterTopology() { update_topology_watch_.Send(); }

std::shared_ptr<Redis> SentinelTopologyHolder::GetRedisInstance(const HostPort& host_port) const {
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

void SentinelTopologyHolder::GetStatistics(SentinelStatistics& stats, const MetricsSettings& settings) const {
    if (sentinels_) {
        stats.sentinel.emplace(ShardStatistics(settings));
        sentinels_->GetStatistics(true, settings, *stats.sentinel);
    }
    stats.internal.is_autotopology = true;
    stats.internal.cluster_topology_checks = utils::statistics::Rate{
        cluster_slots_call_counter_.load(std::memory_order_relaxed)
    };
    stats.internal.cluster_topology_updates = utils::statistics::Rate{
        current_topology_version_.load(std::memory_order_relaxed)
    };

    auto topology = GetTopology();
    topology->GetStatistics(settings, stats);
}

void SentinelTopologyHolder::SetCommandsBufferingSettings(CommandsBufferingSettings settings) {
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

void SentinelTopologyHolder::SetReplicationMonitoringSettings(ReplicationMonitoringSettings settings) {
    {
        auto settings_ptr = monitoring_settings_.Lock();
        *settings_ptr = settings;
    }
    for (const auto& node : nodes_) {
        node.second->SetReplicationMonitoringSettings(settings);
    }
}

void SentinelTopologyHolder::SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) {
    {
        auto settings_ptr = retry_budget_settings_.Lock();
        *settings_ptr = settings;
    }
    for (const auto& node : nodes_) {
        node.second->SetRetryBudgetSettings(settings);
    }
}

void SentinelTopologyHolder::SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) {
    sentinels_->SetConnectionInfo(info_array);
}

boost::signals2::signal<void(SentinelTopologyHolder::HostPort, Redis::State)>&
SentinelTopologyHolder::GetSignalNodeStateChanged() {
    return signal_node_state_change_;
}

boost::signals2::signal<void(size_t)>& SentinelTopologyHolder::GetSignalTopologyChanged() {
    return signal_topology_changed_;
}

void SentinelTopologyHolder::UpdatePassword(const Password& password) {
    auto lock = password_.UniqueLock();
    *lock = password;
}

Password SentinelTopologyHolder::GetPassword() {
    const auto lock = password_.Lock();
    return *lock;
}

std::string SentinelTopologyHolder::GetReadinessInfo() const {
    return fmt::format("Nodes received: false; topology received: {}.", is_topology_received_.load());
}

std::shared_ptr<RedisConnectionHolder> SentinelTopologyHolder::CreateRedisInstance(const std::string& host_port) {
    const auto port_it = host_port.rfind(':');
    UINVARIANT(port_it != std::string::npos, "port must be delimited by ':'");
    const auto port_str = host_port.substr(port_it + 1);
    const auto port = std::stoi(port_str);
    const auto host = host_port.substr(0, port_it);
    const auto buffering_settings_ptr = commands_buffering_settings_.Lock();
    const auto replication_monitoring_settings_ptr = monitoring_settings_.Lock();
    const auto retry_budget_settings_ptr = retry_budget_settings_.Lock();
    LOG_DEBUG() << log_extra_ << "Create new redis instance " << host_port;
    auto creation_settings = RedisConnectionHolder::makeSentinelNodeRedisCreationSettings();
    creation_settings.connection_security = connection_security_;
    return RedisConnectionHolder::Create(
        ev_thread_,
        redis_thread_pool_,
        shard_group_name_,
        host,
        port,
        GetPassword(),
        database_index_,
        buffering_settings_ptr->value_or(CommandsBufferingSettings{}),
        *replication_monitoring_settings_ptr,
        *retry_budget_settings_ptr,
        creation_settings
    );
}

void SentinelTopologyHolder::CreateInstancesAndUpdateTopology() {
    auto release_on_exit_scope = std::move(this->update_topology_guard_);
    ClusterShardHostInfos info;
    {
        auto ptr = new_shard_host_info_.Lock();
        std::swap(*ptr, info);
    }
    // Create missing nodes
    for (const auto& i : info) {
        const auto& host_port = i.master.Fulltext();
        const auto& node = nodes_.Get(host_port);
        if (!node) {
            auto instance = CreateRedisInstance(host_port);
            instance->signal_state_change.connect([host_port, this](redis::RedisState state) {
                GetSignalNodeStateChanged()(host_port, state);
                {
                    const std::lock_guard lock{mutex_};
                }  // do not lose the notify
                cv_.NotifyAll();
            });
            nodes_.Insert(host_port, instance);
        }
        for (const auto& replica : i.slaves) {
            const auto& host_port = replica.Fulltext();
            const auto& node = nodes_.Get(host_port);
            if (!node) {
                auto instance = CreateRedisInstance(host_port);
                instance->signal_state_change.connect([host_port, this](redis::RedisState state) {
                    GetSignalNodeStateChanged()(host_port, state);
                    {
                        const std::lock_guard lock{mutex_};
                    }  // do not lose the notify
                    cv_.NotifyAll();
                });
                nodes_.Insert(host_port, instance);
            }
        }
    }

    // Create new topology
    auto topology = ClusterTopology(
        ++current_topology_version_,
        std::chrono::steady_clock::now(),
        std::move(info),
        redis_thread_pool_,
        nodes_
    );

    LOG_INFO() << log_extra_ << [&infos_list = topology.GetShardInfos()](auto& out) {
        out << "New Redis sentinel topology: ";
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

/// Method that updates topology_ similar to method in userver/redis/src/storages/redis/cluster_sentinel_impl.cpp but
/// uses ProcessGetHostsRequest from SentinelImpl::ReadSentinels() method of
/// userver/redis/src/storages/redis/impl/sentinel_impl.cpp because it is method for redis with sentinels and we want to
/// know what nodes does it have.
void SentinelTopologyHolder::UpdateClusterTopology() {
    if (update_topology_flag_.exchange(true)) {
        return;
    }

    auto reset_update_topology_flag = MakeSharedScopeGuard([&]() { update_topology_flag_ = false; });

    ProcessGetHostsRequest(
        GetHostsRequest::QuerySentinelMasters(*sentinels_, GetPassword()),
        [this,
         reset{std::move(reset_update_topology_flag)
         }](const ConnInfoByShard& info, size_t requests_sent, size_t responses_parsed) mutable {
            if (!CheckQuorum(requests_sent, responses_parsed)) {
                LOG_WARNING()
                    << log_extra_ << "Too many 'sentinel masters' requests failed: requests_sent=" << requests_sent
                    << " responses_parsed=" << responses_parsed;
                return;
            }
            cluster_slots_call_counter_.fetch_add(1, std::memory_order_relaxed);
            if (info.empty()) {
                LOG_WARNING() << log_extra_ << "No masters found by 'sentinel masters'";
                return;
            }

            struct WatchContext {
                std::mutex mutex;
                ClusterShardHostInfos host_infos;
                int counter{0};
            };

            auto watcher = std::make_shared<WatchContext>();
            std::vector<uint8_t> shard_found(shard_by_name_.size(), false);
            for (auto i : info) {
                /// TODO: research what happens if we do not skip unknown shard_names. Look for kRedisSettingsJsonFormat
                const auto it = shard_by_name_.find(i.Name());
                if (it == shard_by_name_.end()) {
                    LOG_INFO()
                        << "Skipped master " << i.Fulltext() << " because name " << i.Name()
                        << " not in shard names list";
                    continue;
                }

                const auto shard_idx = it->second;
                i.SetDatabaseIndex(database_index_);
                i.SetConnectionSecurity(connection_security_);
                watcher->host_infos.push_back(ClusterShardHostInfo{std::move(i), {}, {}});
                shard_found[shard_idx] = true;
            }

            for (size_t idx = 0; idx < shard_by_name_.size(); ++idx) {
                if (!shard_found[idx]) {
                    LOG_WARNING()
                        << log_extra_ << "Shard with name=" << name_by_shard_[idx]
                        << " was not found in 'SENTINEL MASTERS' reply for "
                           "shard_group_name="
                        << shard_group_name_
                        << ". If problem persists, check service and "
                           "redis-sentinel configs.";
                }
            }

            watcher->counter = watcher->host_infos.size();

            for (auto& shard_conn : watcher->host_infos) {
                const auto& shard_name = shard_conn.master.Name();
                ProcessGetHostsRequest(
                    GetHostsRequest::QuerySentinelSlaves(*sentinels_, shard_name, GetPassword()),
                    [this,
                     watcher,
                     shard_name,
                     &shard_conn,
                     reset](const ConnInfoByShard& info, size_t requests_sent, size_t responses_parsed) mutable {
                        if (!CheckQuorum(requests_sent, responses_parsed)) {
                            LOG_WARNING()
                                << log_extra_
                                << "Too many 'sentinel slaves' requests "
                                   "failed: requests_sent="
                                << requests_sent << " responses_parsed=" << responses_parsed;
                            return;
                        }

                        auto it = shard_by_name_.find(shard_name);
                        if (it == shard_by_name_.end()) {
                            LOG_WARNING()
                                << log_extra_ << "Shard name " << shard_name << " not found in shard names list";
                            return;
                        }

                        const std::lock_guard<std::mutex> lock(watcher->mutex);
                        for (auto replica_conn : info) {
                            replica_conn.SetName(shard_name);
                            replica_conn.SetReadOnly(true);
                            replica_conn.SetConnectionSecurity(connection_security_);
                            replica_conn.SetDatabaseIndex(database_index_);
                            shard_conn.slaves.emplace_back(std::move(replica_conn));
                        }

                        if (!--watcher->counter) {
                            const auto cur_topo = topology_.Read();
                            if (!cur_topo.Get() || !cur_topo->HasSameInfos(watcher->host_infos)) {
                                {
                                    auto ptr = new_shard_host_info_.Lock();
                                    std::swap(*ptr, watcher->host_infos);
                                }
                                this->update_topology_guard_ = reset;
                                create_instances_and_update_topology_watch_.Send();
                            }
                        }
                    }
                );
            }
        }
    );
}

}  // namespace storages::redis::impl

// NOLINTEND(clang-analyzer-cplusplus.NewDelete)

USERVER_NAMESPACE_END
