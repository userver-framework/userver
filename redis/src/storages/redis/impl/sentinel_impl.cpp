#include <storages/redis/impl/sentinel_impl.hpp>

#include <algorithm>
#include <sstream>
#include <thread>

#include <boost/crc.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <hiredis/hiredis.h>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/redis_config.hpp>
#include <userver/storages/redis/reply.hpp>

#include "command_control_impl.hpp"

#include <dynamic_config/variables/REDIS_DEADLINE_PROPAGATION_VERSION.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {
namespace {

bool CheckQuorum(size_t requests_sent, size_t responses_parsed) {
    const size_t quorum = requests_sent / 2 + 1;
    return responses_parsed >= quorum;
}

struct CommandSpecialPrinter {
    const CommandPtr& command;
};

logging::LogHelper& operator<<(logging::LogHelper& os, CommandSpecialPrinter v) {
    const auto& command = v.command;

    if (command->args.GetCommandCount() == 1 || command->invoke_counter + 1 >= command->args.GetCommandCount()) {
        os << command->args;
    } else if (command->invoke_counter < command->args.GetCommandCount()) {
        os << fmt::format(
            "subrequest idx={}, cmd={}", command->invoke_counter, command->args.GetCommandName(command->invoke_counter)
        );
    }

    return os;
}

}  // namespace

SentinelImpl::SentinelImpl(
    const engine::ev::ThreadControl& sentinel_thread_control,
    const std::shared_ptr<engine::ev::ThreadPool>& redis_thread_pool,
    Sentinel& sentinel,
    const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns,
    std::string shard_group_name,
    const std::string& client_name,
    const Password& password,
    ConnectionSecurity connection_security,
    std::unique_ptr<KeyShard>&& key_shard,
    dynamic_config::Source dynamic_config_source,
    std::size_t database_index
)
    : sentinel_obj_(sentinel),
      ev_thread_(sentinel_thread_control),
      shard_group_name_(std::move(shard_group_name)),
      init_shards_(std::make_shared<const std::vector<std::string>>(shards)),
      conns_(conns),
      redis_thread_pool_(redis_thread_pool),
      client_name_(client_name),
      connection_security_(connection_security),
      check_interval_(ToEvDuration(kSentinelGetHostsCheckInterval)),
      key_shard_(std::move(key_shard)),
      dynamic_config_source_(dynamic_config_source),
      database_index_(database_index) {
    // https://github.com/boostorg/signals2/issues/59
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
    UASSERT_MSG(key_shard_, "key_shard should be provided");
    for (size_t i = 0; i < init_shards_->size(); ++i) {
        shards_[(*init_shards_)[i]] = i;
        connected_statuses_.push_back(std::make_unique<ConnectedStatus>());
    }
    UpdatePassword(password);

    Init();
    LOG_DEBUG() << "Created SentinelImpl, shard_group_name=" << shard_group_name_ << ", cluster_mode=false";
}

SentinelImpl::~SentinelImpl() { Stop(); }

std::unordered_map<ServerId, size_t, ServerIdHasher>
SentinelImpl::GetAvailableServersWeighted(size_t shard_idx, bool with_master, const CommandControl& cc) const {
    return master_shards_.at(shard_idx)->GetAvailableServersWeighted(with_master, cc);
}

void SentinelImpl::WaitConnectedDebug(bool allow_empty_slaves) {
    static const auto timeout = std::chrono::milliseconds(50);

    for (;; std::this_thread::sleep_for(timeout)) {
        const std::lock_guard sentinels_lock{sentinels_mutex_};

        bool connected_all = true;
        for (const auto& shard : master_shards_)
            connected_all = connected_all && shard->IsConnectedToAllServersDebug(allow_empty_slaves);
        if (connected_all) return;
    }
}

void SentinelImpl::WaitConnectedOnce(RedisWaitConnected wait_connected) {
    auto deadline = engine::Deadline::FromDuration(wait_connected.timeout);

    for (size_t i = 0; i < connected_statuses_.size(); ++i) {
        auto& shard = *connected_statuses_[i];
        if (!shard.WaitReady(deadline, wait_connected.mode)) {
            const std::string msg =
                "Failed to connect to redis, shard_group_name=" + shard_group_name_ + ", shard=" + (*init_shards_)[i] +
                " in " + std::to_string(wait_connected.timeout.count()) + " ms, mode=" + ToString(wait_connected.mode);
            if (wait_connected.throw_on_fail)
                throw ClientNotConnectedException(msg);
            else
                LOG_ERROR() << msg << ", starting with not ready Redis client";
        }
    }
}

void SentinelImpl::ForceUpdateHosts() { ev_thread_.Send(watch_create_); }

void SentinelImpl::Init() {
    InitShards(*init_shards_, master_shards_);

    Shard::Options shard_options;
    shard_options.shard_name = "(sentinel)";
    shard_options.shard_group_name = shard_group_name_;
    shard_options.cluster_mode = false;
    shard_options.ready_change_callback = [this](bool ready) {
        if (ready) ev_thread_.Send(watch_create_);
    };
    shard_options.connection_infos = conns_;
    sentinels_ = std::make_shared<Shard>(std::move(shard_options));
    // https://github.com/boostorg/signals2/issues/59
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
    sentinels_->SignalInstanceStateChange().connect([this](ServerId id, Redis::State state) {
        LOG_TRACE() << "Signaled server " << id.GetDescription() << " state=" << StateToString(state);
        if (state != Redis::State::kInit) ev_thread_.Send(watch_state_);
    });
}

void SentinelImpl::InitShards(
    const std::vector<std::string>& shards,
    std::vector<std::shared_ptr<Shard>>& shard_objects
) {
    size_t i = 0;
    shard_objects.clear();
    for (const auto& shard : shards) {
        Shard::Options shard_options;
        shard_options.shard_name = shard;
        shard_options.shard_group_name = shard_group_name_;
        shard_options.cluster_mode = false;
        shard_options.ready_change_callback = [i, shard](bool ready) {
            LOG_INFO() << "redis: ready_callback:"
                       << "  shard = " << i << "  shard_name = " << shard << "  ready = " << (ready ? "true" : "false");
        };
        auto object = std::make_shared<Shard>(std::move(shard_options));
        // https://github.com/boostorg/signals2/issues/59
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
        object->SignalInstanceStateChange().connect([this](ServerId, Redis::State state) {
            if (state != Redis::State::kInit) ev_thread_.Send(watch_state_);
        });
        // https://github.com/boostorg/signals2/issues/59
        // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDelete)
        object->SignalInstanceReady().connect([this, i](ServerId, bool read_only) {
            LOG_TRACE() << "Signaled kConnected to sentinel: shard_idx=" << i << ", master=" << !read_only;
            if (!read_only)
                connected_statuses_[i]->SetMasterReady();
            else
                connected_statuses_[i]->SetSlaveReady();
        });
        shard_objects.emplace_back(std::move(object));
        ++i;
    }
}

namespace {

void InvokeCommand(CommandPtr command, ReplyPtr&& reply) {
    UASSERT(reply);
    const CommandControlImpl cc{command->control};
    if (reply->server_id.IsAny()) {
        reply->server_id = cc.force_server_id;
    }
    LOG_DEBUG() << "redis_request( " << CommandSpecialPrinter{command}
                << " ):" << (reply->status == ReplyStatus::kOk ? '+' : '-') << ":" << reply->time * 1000.0
                << " cc: " << command->control.ToString() << command->GetLogExtra();
    ++command->invoke_counter;
    try {
        command->callback(command, reply);
    } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in command->callback, cmd=" << reply->cmd << " " << ex << command->log_extra;
    } catch (...) {
        LOG_WARNING() << "exception in command->callback, cmd=" << reply->cmd << command->log_extra;
    }
}

std::optional<std::chrono::milliseconds> GetDeadlineTimeLeft() {
    if (!engine::current_task::IsTaskProcessorThread()) return std::nullopt;

    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
    if (!inherited_deadline.IsReachable()) return std::nullopt;

    const auto inherited_timeout =
        std::chrono::duration_cast<std::chrono::milliseconds>(inherited_deadline.TimeLeftApprox());
    return inherited_timeout;
}

}  // namespace

bool AdjustDeadline(const SentinelImplBase::SentinelCommand& scommand, const dynamic_config::Snapshot& config) {
    const auto inherited_deadline = GetDeadlineTimeLeft();
    if (!inherited_deadline) return true;

    if (config[::dynamic_config::REDIS_DEADLINE_PROPAGATION_VERSION] != kDeadlinePropagationExperimentVersion) {
        return true;
    }

    if (*inherited_deadline <= std::chrono::seconds{0}) {
        return false;
    }

    auto& cc = scommand.command->control;
    if (!cc.timeout_single || *cc.timeout_single > *inherited_deadline) cc.timeout_single = *inherited_deadline;
    if (!cc.timeout_all || *cc.timeout_all > *inherited_deadline) cc.timeout_all = *inherited_deadline;

    return true;
}

void SentinelImpl::AsyncCommand(const SentinelCommand& scommand, size_t prev_instance_idx) {
    if (!AdjustDeadline(scommand, dynamic_config_source_.GetSnapshot())) {
        auto reply =
            std::make_shared<Reply>("", ReplyData::CreateError("Deadline propagation"), ReplyStatus::kTimeoutError);
        InvokeCommand(scommand.command, std::move(reply));
        return;
    }

    const CommandPtr command = scommand.command;
    UINVARIANT(!command->asking, "ASK command requested in non cluster SentinelImpl");
    const std::size_t shard = (scommand.shard == kUnknownShard ? 0 : scommand.shard);
    const bool master = scommand.master;

    const auto start = scommand.start;
    const auto counter = command->counter;
    const CommandPtr command_check_errors(PrepareCommand(
        std::move(command->args),
        [this, shard, master, start, counter, command](const CommandPtr& ccommand, ReplyPtr reply) {
            if (counter != command->counter) return;
            UASSERT(reply);

            const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

            UASSERT_MSG(!reply->data.IsErrorAsk(), "ASK command error in non cluster SentinelImpl");
            UASSERT_MSG(!reply->data.IsErrorMoved(), "MOVED error in non cluster SentinelImpl");
            const bool retry_to_master =
                !master && reply->data.IsNil() && command->control.force_retries_to_master_on_nil_reply;
            const bool retry = retry_to_master || reply->status != ReplyStatus::kOk ||
                               reply->IsUnusableInstanceError() || reply->IsReadonlyError();

            if (retry) {
                const CommandControlImpl cc{command->control};
                const std::size_t retries_left = cc.max_retries - 1;
                const std::chrono::steady_clock::time_point until = start + cc.timeout_all;
                if (now < until && retries_left > 0) {
                    const auto timeout_all = std::chrono::duration_cast<std::chrono::milliseconds>(until - now);
                    command->control.timeout_single = std::min(cc.timeout_single, timeout_all);
                    command->control.timeout_all = timeout_all;
                    command->control.max_retries = retries_left;

                    auto new_command = PrepareCommand(
                        ccommand->args.Clone(),
                        [command](const CommandPtr& cmd, ReplyPtr reply) {
                            if (command->callback) command->callback(cmd, std::move(reply));
                        },
                        command->control,
                        command->counter + 1,
                        command->asking,
                        0,
                        false  // error_ask || error_moved
                    );
                    new_command->log_extra = std::move(command->log_extra);
                    AsyncCommand(
                        SentinelCommand(new_command, master || retry_to_master, shard, start), ccommand->instance_idx
                    );
                    return;
                }
            }

            const std::chrono::duration<double> time = now - start;
            reply->time = time.count();
            command->args = std::move(ccommand->args);
            InvokeCommand(command, std::move(reply));
            ccommand->args = std::move(command->args);
        },
        command->control,
        command->counter,
        command->asking,
        prev_instance_idx,
        false,
        !master
    ));

    UASSERT(shard < master_shards_.size());
    auto master_shard = master_shards_[shard];
    if (!master_shard->AsyncCommand(command_check_errors)) {
        scommand.command->args = std::move(command_check_errors->args);
        AsyncCommandFailed(scommand);
        return;
    }
}

size_t SentinelImpl::ShardByKey(const std::string& key) const {
    UASSERT(!master_shards_.empty());
    const size_t shard = key_shard_->ShardByKey(key);
    LOG_TRACE() << "key=" << key << " shard=" << shard;
    return shard;
}

void SentinelImpl::Start() {
    watch_state_.data = this;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&watch_state_, ChangedState);
    ev_thread_.Start(watch_state_);

    watch_update_.data = this;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&watch_update_, UpdateInstances);
    ev_thread_.Start(watch_update_);

    watch_create_.data = this;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_async_init(&watch_create_, OnModifyConnectionInfo);
    ev_thread_.Start(watch_create_);

    check_timer_.data = this;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_init(&check_timer_, OnCheckTimer, 0.0, 0.0);
    ev_thread_.Start(check_timer_);
}

void SentinelImpl::AsyncCommandFailed(const SentinelCommand& scommand) {
    // Run command callbacks from redis thread only.
    // It prevents recursive mutex locking in subscription_storage.
    EnqueueCommand(scommand);
}

void SentinelImpl::OnCheckTimer(struct ev_loop*, ev_timer* w, int) noexcept {
    auto* impl = static_cast<SentinelImpl*>(w->data);
    UASSERT(impl != nullptr);
    impl->RefreshConnectionInfo();
}

void SentinelImpl::ChangedState(struct ev_loop*, ev_async* w, int) noexcept {
    auto* impl = static_cast<SentinelImpl*>(w->data);
    UASSERT(impl != nullptr);
    impl->CheckConnections();
}

void SentinelImpl::ProcessCreationOfShards(std::vector<std::shared_ptr<Shard>>& shards) {
    for (size_t shard_idx = 0; shard_idx < shards.size(); shard_idx++) {
        auto& info = *shards[shard_idx];
        if (info.ProcessCreation(redis_thread_pool_)) {
            sentinel_obj_.NotifyInstancesChanged(shard_idx);
        }
    }
}

void SentinelImpl::RefreshConnectionInfo() {
    try {
        sentinels_->ProcessCreation(redis_thread_pool_);

        ProcessCreationOfShards(master_shards_);
        ReadSentinels();
    } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in RefreshConnectionInfo: " << ex.what();
    }

    /* FIXME: this should be called not every check_interval_,
     * but using a min deadline time of all waiting commands.
     * Replies will be handled in 3 seconds instead of lower timeout with current
     * logic.
     * */
    ProcessWaitingCommands();

    ev_thread_.Stop(check_timer_);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_timer_set(&check_timer_, check_interval_, 0.0);
    ev_thread_.Start(check_timer_);
}

void SentinelImpl::Stop() {
    UASSERT(engine::current_task::IsTaskProcessorThread());

    ev_thread_.RunInEvLoopBlocking([this] {
        ev_thread_.Stop(check_timer_);
        ev_thread_.Stop(watch_state_);
        ev_thread_.Stop(watch_update_);
        ev_thread_.Stop(watch_create_);

        auto clean_shards = [](std::vector<std::shared_ptr<Shard>>& shards) {
            for (auto& shard : shards) shard->Clean();
        };
        {
            const std::lock_guard<std::mutex> lock(command_mutex_);
            while (!commands_.empty()) {
                auto command = commands_.back().command;
                for (const auto& args : command->args) {
                    LOG_ERROR() << fmt::format("Killing request: {}", args.GetJoinedArgs(", "));
                    auto reply = std::make_shared<Reply>(
                        args.GetCommandName(),
                        ReplyData::CreateError("Stopping, killing commands remaining in send queue"),
                        ReplyStatus::kEndOfFileError
                    );
                    statistics_internal_.redis_not_ready++;
                    InvokeCommand(command, std::move(reply));
                }
                commands_.pop_back();
            }
        }
        clean_shards(master_shards_);
        sentinels_->Clean();
    });
}

void SentinelImpl::SetCommandsBufferingSettings(CommandsBufferingSettings commands_buffering_settings) {
    if (commands_buffering_settings_ && *commands_buffering_settings_ == commands_buffering_settings) return;
    commands_buffering_settings_ = commands_buffering_settings;
    for (auto& shard : master_shards_) shard->SetCommandsBufferingSettings(commands_buffering_settings);
}

void SentinelImpl::SetReplicationMonitoringSettings(const ReplicationMonitoringSettings& replication_monitoring_settings
) {
    for (auto& shard : master_shards_) shard->SetReplicationMonitoringSettings(replication_monitoring_settings);
}

void SentinelImpl::SetRetryBudgetSettings(const utils::RetryBudgetSettings& retry_budget_settings) {
    for (auto& shard : master_shards_) shard->SetRetryBudgetSettings(retry_budget_settings);
}

PublishSettings SentinelImpl::GetPublishSettings() {
    /// Why do we always publish to master? We can actually publish to any host in
    /// shard to distribute load evenly
    return PublishSettings{++publish_shard_ % ShardsCount(), true, CommandControl::Strategy::kDefault};
}

void SentinelImpl::ReadSentinels() {
    ProcessGetHostsRequest(
        GetHostsRequest(*sentinels_, GetPassword()),
        [this](const ConnInfoByShard& info, size_t requests_sent, size_t responses_parsed) {
            if (!CheckQuorum(requests_sent, responses_parsed)) {
                LOG_WARNING() << "Too many 'sentinel masters' requests failed: requests_sent=" << requests_sent
                              << " responses_parsed=" << responses_parsed;
                return;
            }
            struct WatchContext {
                ConnInfoByShard masters;
                ConnInfoByShard slaves;
                std::mutex mutex;
                int counter{0};
                ShardInfo::HostPortToShardMap host_port_to_shard;
            };
            auto watcher = std::make_shared<WatchContext>();

            std::vector<uint8_t> shard_found(init_shards_->size(), false);
            for (auto shard_conn : info) {
                if (shards_.find(shard_conn.Name()) != shards_.end()) {
                    shard_conn.SetConnectionSecurity(connection_security_);
                    shard_conn.SetDatabaseIndex(database_index_);
                    shard_found[shards_[shard_conn.Name()]] = true;
                    watcher->host_port_to_shard[shard_conn.HostPort()] = shards_[shard_conn.Name()];
                    watcher->masters.push_back(std::move(shard_conn));
                }
            }

            for (size_t idx = 0; idx < shard_found.size(); idx++) {
                if (!shard_found[idx]) {
                    LOG_WARNING() << "Shard with name=" << (*init_shards_)[idx]
                                  << " was not found in 'SENTINEL MASTERS' reply for "
                                     "shard_group_name="
                                  << shard_group_name_
                                  << ". If problem persists, check service and "
                                     "redis-sentinel configs.";
                }
            }
            watcher->counter = watcher->masters.size();

            for (const auto& shard_conn : watcher->masters) {
                const auto& shard = shard_conn.Name();
                ProcessGetHostsRequest(
                    GetHostsRequest(*sentinels_, shard_conn.Name(), GetPassword()),
                    [this, watcher, shard](const ConnInfoByShard& info, size_t requests_sent, size_t responses_parsed) {
                        if (!CheckQuorum(requests_sent, responses_parsed)) {
                            LOG_WARNING() << "Too many 'sentinel slaves' requests "
                                             "failed: requests_sent="
                                          << requests_sent << " responses_parsed=" << responses_parsed;
                            return;
                        }
                        const std::lock_guard<std::mutex> lock(watcher->mutex);
                        for (auto shard_conn : info) {
                            shard_conn.SetName(shard);
                            shard_conn.SetReadOnly(true);
                            shard_conn.SetConnectionSecurity(connection_security_);
                            shard_conn.SetDatabaseIndex(database_index_);
                            if (shards_.find(shard_conn.Name()) != shards_.end())
                                watcher->host_port_to_shard[shard_conn.HostPort()] = shards_[shard_conn.Name()];
                            watcher->slaves.push_back(std::move(shard_conn));
                        }
                        if (!--watcher->counter) {
                            shard_info_.UpdateHostPortToShard(std::move(watcher->host_port_to_shard));

                            {
                                const std::lock_guard<std::mutex> lock_this(sentinels_mutex_);
                                master_shards_info_.swap(watcher->masters);
                                slaves_shards_info_.swap(watcher->slaves);
                            }
                            ev_thread_.Send(watch_update_);
                        }
                    }
                );
            }
        }
    );
}

void SentinelImpl::UpdateInstances(struct ev_loop*, ev_async* w, int) noexcept {
    auto* impl = static_cast<SentinelImpl*>(w->data);
    UASSERT(impl != nullptr);
    impl->UpdateInstancesImpl();
}

bool SentinelImpl::SetConnectionInfo(ConnInfoMap info_by_shards, std::vector<std::shared_ptr<Shard>>& shards) {
    /* Ensure all shards are in info_by_shards to remove the last instance
     * from the empty shard */
    for (const auto& it : shards_) {
        info_by_shards[it.first];
    }

    bool res = false;
    for (const auto& info_iterator : info_by_shards) {
        auto j = shards_.find(info_iterator.first);
        if (j != shards_.end()) {
            const size_t shard = j->second;
            auto shard_ptr = shards[shard];
            const bool changed = shard_ptr->SetConnectionInfo(info_iterator.second);

            if (changed) {
                std::vector<std::string> conn_strs;
                conn_strs.reserve(info_iterator.second.size());
                for (const auto& conn_str : info_iterator.second) conn_strs.push_back(conn_str.Fulltext());
                LOG_INFO() << "Redis state changed for client=" << client_name_ << " shard=" << j->first
                           << ", now it is " << fmt::to_string(fmt::join(conn_strs, ", "))
                           << ", connections=" << shard_ptr->InstancesSize();
                res = true;
            }
        }
    }
    return res;
}

void SentinelImpl::UpdateInstancesImpl() {
    bool changed = false;
    {
        const std::lock_guard<std::mutex> lock(sentinels_mutex_);
        ConnInfoMap info_map;
        for (const auto& info : master_shards_info_) info_map[info.Name()].emplace_back(info);
        for (const auto& info : slaves_shards_info_) info_map[info.Name()].emplace_back(info);
        changed |= SetConnectionInfo(std::move(info_map), master_shards_);
    }
    if (changed) ev_thread_.Send(watch_create_);
}

void SentinelImpl::OnModifyConnectionInfo(struct ev_loop*, ev_async* w, int) noexcept {
    auto* impl = static_cast<SentinelImpl*>(w->data);
    UASSERT(impl != nullptr);
    impl->RefreshConnectionInfo();
}

void SentinelImpl::CheckConnections() {
    sentinels_->ProcessStateUpdate();

    for (size_t shard_idx = 0; shard_idx < master_shards_.size(); shard_idx++) {
        const auto& info = master_shards_[shard_idx];
        if (info->ProcessStateUpdate()) {
            sentinel_obj_.NotifyInstancesChanged(shard_idx);
        }
    }

    ProcessWaitingCommands();
}

void SentinelImpl::ProcessWaitingCommands() {
    std::vector<SentinelCommand> waiting_commands;

    {
        const std::lock_guard<std::mutex> lock(command_mutex_);
        waiting_commands.swap(commands_);
    }
    if (!waiting_commands.empty()) {
        LOG_INFO() << "ProcessWaitingCommands client=" << client_name_ << " shard_group_name=" << shard_group_name_
                   << " waiting_commands.size()=" << waiting_commands.size();
    }

    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    for (const SentinelCommand& scommand : waiting_commands) {
        const auto& command = scommand.command;
        const CommandControlImpl cc{command->control};
        if (scommand.start + cc.timeout_all < now) {
            for (const auto& args : command->args) {
                auto reply = std::make_shared<Reply>(
                    args.GetCommandName(),
                    ReplyData::CreateError("Command in the send queue timed out"),
                    ReplyStatus::kTimeoutError
                );
                statistics_internal_.redis_not_ready++;
                InvokeCommand(command, std::move(reply));
            }
        } else {
            AsyncCommand(scommand, kDefaultPrevInstanceIdx);
        }
    }
}

Password SentinelImpl::GetPassword() {
    const auto lock = password_.Lock();
    return *lock;
}

SentinelStatistics SentinelImpl::GetStatistics(const MetricsSettings& settings) const {
    SentinelStatistics stats(settings, statistics_internal_);
    const std::lock_guard<std::mutex> lock(sentinels_mutex_);
    for (const auto& shard : master_shards_) {
        if (!shard) continue;
        auto masters_it = stats.masters.emplace(shard->ShardName(), ShardStatistics(settings));
        auto& master_stats = masters_it.first->second;
        shard->GetStatistics(true, settings, master_stats);
        stats.shard_group_total.Add(master_stats.shard_total);

        auto slave_it = stats.slaves.emplace(shard->ShardName(), ShardStatistics(settings));
        auto& slave_stats = slave_it.first->second;
        shard->GetStatistics(false, settings, slave_stats);
        stats.shard_group_total.Add(slave_stats.shard_total);
    }
    stats.sentinel.emplace(ShardStatistics(settings));
    sentinels_->GetStatistics(true, settings, *stats.sentinel);

    return stats;
}

void SentinelImpl::EnqueueCommand(const SentinelCommand& command) {
    const std::lock_guard<std::mutex> lock(command_mutex_);
    commands_.push_back(command);
}

size_t SentinelImpl::ShardInfo::GetShard(const std::string& host, int port) const {
    const std::lock_guard<std::mutex> lock(mutex_);

    const auto it = host_port_to_shard_.find(std::make_pair(host, port));
    if (it == host_port_to_shard_.end()) return kUnknownShard;
    return it->second;
}

void SentinelImpl::ShardInfo::UpdateHostPortToShard(HostPortToShardMap&& host_port_to_shard_new) {
    const std::lock_guard<std::mutex> lock(mutex_);
    if (host_port_to_shard_new != host_port_to_shard_) {
        host_port_to_shard_.swap(host_port_to_shard_new);
    }
}

void SentinelImpl::ConnectedStatus::SetMasterReady() {
    if (!master_ready_.exchange(true)) {
        { const std::lock_guard lock{mutex_}; }  // do not lose the notify
        cv_.NotifyAll();
    }
}

void SentinelImpl::ConnectedStatus::SetSlaveReady() {
    if (!slave_ready_.exchange(true)) {
        { const std::lock_guard lock{mutex_}; }  // do not lose the notify
        cv_.NotifyAll();
    }
}

bool SentinelImpl::ConnectedStatus::WaitReady(engine::Deadline deadline, WaitConnectedMode mode) {
    switch (mode) {
        case WaitConnectedMode::kNoWait:
            return true;
        case WaitConnectedMode::kMaster:
            return Wait(deadline, [this] { return master_ready_.load(); });
        case WaitConnectedMode::kSlave:
            return Wait(deadline, [this] { return slave_ready_.load(); });
        case WaitConnectedMode::kMasterOrSlave:
            return Wait(deadline, [this] { return master_ready_ || slave_ready_; });
        case WaitConnectedMode::kMasterAndSlave:
            return Wait(deadline, [this] { return master_ready_ && slave_ready_; });
    }
    throw std::runtime_error("unknown mode: " + std::to_string(static_cast<int>(mode)));
}

template <typename Pred>
bool SentinelImpl::ConnectedStatus::Wait(engine::Deadline deadline, const Pred& pred) {
    std::unique_lock<std::mutex> lock(mutex_);
    return cv_.WaitUntil(lock, deadline, pred);
}

void SentinelImpl::SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) {
    sentinels_->SetConnectionInfo(info_array);
}

void SentinelImpl::UpdatePassword(const Password& password) {
    auto lock = password_.UniqueLock();
    *lock = password;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
