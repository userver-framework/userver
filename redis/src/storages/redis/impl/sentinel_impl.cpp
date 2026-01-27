#include "sentinel_impl.hpp"

#include <atomic>

#include <fmt/format.h>
#include <boost/container_hash/hash.hpp>
#include <boost/crc.hpp>

#include <userver/concurrent/variable.hpp>
#include <userver/logging/log.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/redis_config.hpp>
#include <userver/storages/redis/redis_state.hpp>
#include <userver/storages/redis/reply.hpp>

#include <engine/ev/watcher.hpp>
#include <engine/ev/watcher/async_watcher.hpp>
#include <engine/ev/watcher/periodic_watcher.hpp>
#include <storages/redis/impl/cluster_topology.hpp>
#include <storages/redis/impl/cluster_topology_holder.hpp>
#include <storages/redis/impl/redis_connection_holder.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/sentinel_topology_holder.hpp>
#include <storages/redis/impl/standalone_topology_holder.hpp>
#include <storages/redis/impl/topology_holder_base.hpp>

#include <dynamic_config/variables/REDIS_DEADLINE_PROPAGATION_VERSION.hpp>

#include "command_control_impl.hpp"

USERVER_NAMESPACE_BEGIN

// https://github.com/boostorg/signals2/issues/59
// NOLINTBEGIN(clang-analyzer-cplusplus.NewDelete)

namespace storages::redis::impl {

namespace {

std::optional<std::chrono::milliseconds> GetDeadlineTimeLeft() {
    if (!engine::current_task::IsTaskProcessorThread()) {
        return std::nullopt;
    }

    const auto inherited_deadline = server::request::GetTaskInheritedDeadline();
    if (!inherited_deadline.IsReachable()) {
        return std::nullopt;
    }

    const auto
        inherited_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(inherited_deadline.TimeLeftApprox());
    return inherited_timeout;
}

bool AdjustDeadline(const SentinelImpl::SentinelCommand& scommand, const dynamic_config::Snapshot& config) {
    const auto inherited_deadline = GetDeadlineTimeLeft();
    if (!inherited_deadline) {
        return true;
    }

    if (config[::dynamic_config::REDIS_DEADLINE_PROPAGATION_VERSION] != kDeadlinePropagationExperimentVersion) {
        return true;
    }

    if (*inherited_deadline <= std::chrono::seconds{0}) {
        return false;
    }

    auto& cc = scommand.command->control;
    if (!cc.timeout_single || *cc.timeout_single > *inherited_deadline) {
        cc.timeout_single = *inherited_deadline;
    }
    if (!cc.timeout_all || *cc.timeout_all > *inherited_deadline) {
        cc.timeout_all = *inherited_deadline;
    }

    return true;
}

size_t HashSlot(const std::string& key) {
    size_t start = 0;
    size_t len = 0;
    GetRedisKey(key, &start, &len);
    return std::for_each(key.data() + start, key.data() + start + len, boost::crc_optimal<16, 0x1021>())() & 0x3fff;
}

std::string ParseMovedShard(const std::string& err_string) {
    static const auto kUnknownShard = std::string("");
    size_t pos = err_string.find(' ');  // skip "MOVED" or "ASK"
    if (pos == std::string::npos) {
        return kUnknownShard;
    }
    pos = err_string.find(' ', pos + 1);  // skip hash_slot
    if (pos == std::string::npos) {
        return kUnknownShard;
    }
    pos++;
    size_t end = err_string.find(' ', pos);
    if (end == std::string::npos) {
        end = err_string.size();
    }
    const size_t colon_pos = err_string.rfind(':', end);
    int port = 0;
    try {
        port = std::stoi(err_string.substr(colon_pos + 1, end - (colon_pos + 1)));
    } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in " << __func__ << "(\"" << err_string << "\") " << ex.what();
        return kUnknownShard;
    }
    return err_string.substr(pos, colon_pos - pos) + ":" + std::to_string(port);
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
            "subrequest idx={}, cmd={}",
            command->invoke_counter,
            command->args.GetCommandName(command->invoke_counter)
        );
    }

    return os;
}

void InvokeCommand(CommandPtr command, ReplyPtr&& reply, const logging::LogExtra& log_extra) {
    UASSERT(reply);
    UASSERT(command);

    if (reply->server_id.IsAny()) {
        reply->server_id = CommandControlImpl{command->control}.force_server_id;
    }
    LOG_DEBUG()
        << log_extra << "redis_request( " << CommandSpecialPrinter{command}
        << " ):" << (reply->status == ReplyStatus::kOk ? '+' : '-') << ":" << reply->time * 1000.0
        << " cc: " << command->control.ToString() << command->GetLogExtra();
    ++command->invoke_counter;
    try {
        command->callback(command, reply);
    } catch (const std::exception& ex) {
        UASSERT(!engine::current_task::IsTaskProcessorThread());
        LOG_WARNING()
            << log_extra << "exception in command->callback, cmd=" << reply->cmd << " " << ex << command->GetLogExtra();
    } catch (...) {
        UASSERT(!engine::current_task::IsTaskProcessorThread());
        LOG_WARNING() << log_extra << "exception in command->callback, cmd=" << reply->cmd << command->GetLogExtra();
    }
}

}  // namespace

void SentinelImpl::ProcessWaitingCommands() {
    std::vector<SentinelCommand> waiting_commands;

    {
        const std::lock_guard<std::mutex> lock(command_mutex_);
        waiting_commands.swap(commands_);
    }
    if (!waiting_commands.empty()) {
        LOG_INFO()
            << log_extra_ << "ProcessWaitingCommands client=" << client_name_
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
                InvokeCommand(command, std::move(reply), log_extra_);
            }
        } else {
            AsyncCommand(scommand, kDefaultPrevInstanceIdx);
        }
    }
}

void SentinelImpl::ProcessWaitingCommandsOnStop() {
    std::vector<SentinelCommand> waiting_commands;

    {
        const std::lock_guard<std::mutex> lock(command_mutex_);
        waiting_commands.swap(commands_);
    }

    for (const SentinelCommand& scommand : waiting_commands) {
        const auto& command = scommand.command;
        for (const auto& args : command->args) {
            auto reply = std::make_shared<Reply>(
                args.GetCommandName(),
                ReplyData::CreateError("Stopping, killing commands remaining in send queue"),
                ReplyStatus::kTimeoutError
            );
            statistics_internal_.redis_not_ready++;
            InvokeCommand(command, std::move(reply), log_extra_);
        }
    }
}

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
    KeyShardFactory&& key_shard_factory,
    dynamic_config::Source dynamic_config_source,
    std::size_t database_index
)
    : sentinel_obj_(sentinel),
      ev_thread_(sentinel_thread_control),
      process_waiting_commands_timer_(std::make_unique<engine::ev::PeriodicWatcher>(
          ev_thread_,
          [this] { ProcessWaitingCommands(); },
          kSentinelGetHostsCheckInterval
      )),
      key_shard_factory_(std::move(key_shard_factory)),
      key_shard_(key_shard_factory_(shards.size())),
      shard_group_name_(std::move(shard_group_name)),
      conns_(conns),
      redis_thread_pool_(redis_thread_pool),
      client_name_(client_name),
      dynamic_config_source_(std::move(dynamic_config_source)),
      database_index_(database_index)
{
    log_extra_.Extend("shard_group_name", shard_group_name_);

    const auto& key_shard_type = key_shard_factory_.GetType();
    topology_holder_ = [&]() -> std::unique_ptr<TopologyHolderBase> {
        if (key_shard_type == "RedisCluster") {
            return std::make_unique<ClusterTopologyHolder>(
                ev_thread_,
                redis_thread_pool,
                shard_group_name_,
                password,
                shards,
                conns,
                connection_security
            );
        } else if (key_shard_type == "RedisStandalone") {
            LOG_DEBUG() << log_extra_ << "Construct Standalone topology holder";
            UASSERT_MSG(conns.size() == 1, "In standalone mode we expect exactly one redis node to connect!");
            // TODO: TAXICOMMON-10376 experiment with providing kClusterDatabaseIndex other than 0 for standalone mode
            return std::make_unique<StandaloneTopologyHolder>(
                ev_thread_,
                redis_thread_pool,
                shard_group_name,
                password,
                database_index_,
                conns.front()
            );
        }

        return std::make_unique<SentinelTopologyHolder>(
            ev_thread_,
            redis_thread_pool,
            shard_group_name_,
            password,
            database_index_,
            shards,
            conns,
            connection_security
        );
    }();
}

SentinelImpl::~SentinelImpl() {
    delete_started_ = true;
    Stop();
}

std::unordered_map<ServerId, size_t, ServerIdHasher> SentinelImpl::GetAvailableServersWeighted(
    size_t /*shard_idx*/,
    bool with_master,
    const CommandControl& cc
) const {
    auto topology = topology_holder_->GetTopology();
    /// Method used only in Subscribe. When using cluster mode every node
    /// can listen messages from any node. So ignore concrete shard and use
    /// kUnknown to get all of cluster nodes.
    return topology->GetAvailableServersWeighted(kUnknownShard, with_master, cc);
}

void SentinelImpl::WaitConnectedDebug(bool allow_empty_slaves) {
    const auto mode = allow_empty_slaves ? WaitConnectedMode::kMaster : WaitConnectedMode::kMasterAndSlave;
    const RedisWaitConnected wait_connected{mode, true, kRedisWaitConnectedDefaultTimeout};
    WaitConnectedOnce(wait_connected);
}

void SentinelImpl::WaitConnectedOnce(RedisWaitConnected wait_connected) {
    auto deadline = engine::Deadline::FromDuration(wait_connected.timeout);
    if (!topology_holder_->WaitReadyOnce(deadline, wait_connected.mode)) {
        auto topology = topology_holder_->GetTopology();
        const std::string msg = fmt::format(
            "Failed to init cluster slots for redis, shard_group_name={} in {} "
            "ms, mode={}. {} {}",
            shard_group_name_,
            wait_connected.timeout.count(),
            ToString(wait_connected.mode),
            topology_holder_->GetReadinessInfo(),
            topology->GetReadinessInfo()
        );
        if (wait_connected.throw_on_fail) {
            throw ClientNotConnectedException(msg);
        } else {
            LOG_WARNING() << log_extra_ << msg << ", starting with not ready Redis client";
        }
    }
}

void SentinelImpl::ForceUpdateHosts() { topology_holder_->SendUpdateClusterTopology(); }

void SentinelImpl::Init() {
    topology_holder_->GetSignalNodeStateChanged().connect([this](std::string host_port, Redis::State /*state*/) {
        const auto topology = topology_holder_->GetTopology();
        const auto shard_opt = topology->GetShardByHostPort(host_port);
        if (!shard_opt) {
            // changed state of node not used in cluster (e.g. no slots assigned)
            return;
        }

        sentinel_obj_.NotifyInstancesChanged(*shard_opt);
    });

    topology_holder_->GetSignalTopologyChanged().connect([this](size_t shards_count) {
        sentinel_obj_.NotifyTopologyChanged(shards_count);
    });

    topology_holder_->Init();
}

void SentinelImpl::AsyncCommand(const SentinelCommand& scommand, size_t prev_instance_idx) {
    if (!AdjustDeadline(scommand, dynamic_config_source_.GetSnapshot())) {
        auto reply = std::make_shared<
            Reply>("", ReplyData::CreateError("Deadline propagation"), ReplyStatus::kTimeoutError);
        InvokeCommand(scommand.command, std::move(reply), log_extra_);
        return;
    }

    const CommandPtr command = scommand.command;
    const size_t shard = scommand.shard;
    const bool master = scommand.master;
    const auto start = scommand.start;
    const auto counter = command->counter;
    const CommandPtr command_check_errors(PrepareCommand(
        std::move(command->args),
        [this, shard, master, start, counter, command](const CommandPtr& ccommand, ReplyPtr reply) {
            if (counter != command->counter) {
                return;
            }
            UASSERT(reply);

            const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();

            const bool error_ask = reply->data.IsErrorAsk();
            const bool error_moved =
                reply->data.IsErrorMoved()
                // *SUBSCRIBE commands have logic in FSM, those commands do not expect retries on move.
                // The behavior is tested in redis/functional_tests/cluster_auto_topology_pubsub/tests
                && ccommand->args.GetCommandCount() == 1 && !ccommand->args.begin()->IsSubscribeCommand();

            if (error_moved) {
                LOG_DEBUG()
                    << log_extra_ << "MOVED c.instance_idx:" << ccommand->instance_idx << " shard: " << shard
                    << " movedto:" << ParseMovedShard(reply->data.GetError()) << " args:" << ccommand->args;
                this->topology_holder_->SendUpdateClusterTopology();
            }
            const bool retry_to_master =
                !master && reply->data.IsNil() && command->control.force_retries_to_master_on_nil_reply;
            const bool retry =
                (retry_to_master || reply->status != ReplyStatus::kOk || error_ask || error_moved ||
                 reply->IsUnusableInstanceError() || reply->IsReadonlyError() || reply->data.IsErrorClusterdown()) &&
                !delete_started_;

            std::shared_ptr<Redis> moved_to_instance;
            if (retry) {
                const CommandControlImpl cc{command->control};
                const size_t new_shard = shard;
                size_t retries_left = cc.max_retries - 1;
                if (error_ask || error_moved) {
                    LOG_DEBUG()
                        << log_extra_ << "Got error '" << reply->data.GetError() << "' reply, cmd=" << reply->cmd
                        << ", server=" << reply->server_id.GetDescription();
                    const auto& host_port = ParseMovedShard(reply->data.GetError());
                    command->counter++;
                    if (!command->redirected || (error_ask && !command->asking)) {
                        ++retries_left;
                    }
                    moved_to_instance = topology_holder_->GetRedisInstance(host_port);
                    if (!moved_to_instance) {
                        LOG_WARNING() << log_extra_ << "moved to unknown host " << host_port;
                        /// Can we do something else?? We don't have client for this
                        /// redis instance
                        return;
                    }
                }
                const std::chrono::steady_clock::time_point until = start + cc.timeout_all;
                if (now < until && retries_left > 0) {
                    const auto timeout_all = std::chrono::duration_cast<std::chrono::milliseconds>(until - now);
                    command->control.timeout_single = std::min(cc.timeout_single, timeout_all);
                    command->control.timeout_all = timeout_all;
                    command->control.max_retries = retries_left;

                    auto new_command = PrepareCommand(
                        ccommand->args.Clone(),
                        [command](const CommandPtr& cmd, ReplyPtr reply) {
                            if (command->callback) {
                                command->callback(cmd, std::move(reply));
                            }
                        },
                        command->control,
                        command->counter + 1,
                        command->asking || error_ask,
                        0,
                        error_ask || error_moved
                    );
                    new_command->log_extra = std::move(command->log_extra);
                    if (moved_to_instance) {
                        moved_to_instance->AsyncCommand(new_command);
                    } else {
                        AsyncCommand(
                            SentinelCommand(
                                new_command,
                                master || retry_to_master || (error_moved && shard == new_shard),
                                new_shard,
                                start
                            ),
                            ccommand->instance_idx
                        );
                    }
                    return;
                }
            }

            const std::chrono::duration<double> time = now - start;
            reply->time = time.count();
            command->args = std::move(ccommand->args);
            InvokeCommand(command, std::move(reply), log_extra_);
            ccommand->args = std::move(command->args);
        },
        command->control,
        command->counter,
        command->asking,
        prev_instance_idx,
        false,
        !master
    ));

    const auto topology = topology_holder_->GetTopology();
    const auto& master_shard = topology->GetClusterShardByIndex(shard);
    if (!master_shard.AsyncCommand(command_check_errors)) {
        scommand.command->args = std::move(command_check_errors->args);
        AsyncCommandFailed(scommand);
        return;
    }
}

size_t SentinelImpl::ShardByKey(const std::string& key) const {
    // TODO: Move this branches inside topologies.
    if (key_shard_) {
        // sentinel/standalone branch
        const auto shard = key_shard_->ShardByKey(key);
        LOG_TRACE() << log_extra_ << "key=" << key << " shard=" << shard;
        return shard;
    }

    // cluster branch
    const auto slot = HashSlot(key);
    const auto ptr = topology_holder_->GetTopology();
    return ptr->GetShardIndexBySlot(slot);
}

void SentinelImpl::Start() {
    Init();
    LOG_DEBUG() << log_extra_ << "Created ClusterSentinelImpl";

    topology_holder_->Start();
    process_waiting_commands_timer_->Start();
}

void SentinelImpl::AsyncCommandFailed(const SentinelCommand& scommand) {
    // Run command callbacks from redis thread only.
    // It prevents recursive mutex locking in subscription_storage.
    EnqueueCommand(scommand);
}

void SentinelImpl::Stop() {
    UASSERT(engine::current_task::IsTaskProcessorThread());
    topology_holder_->Stop();
    ev_thread_.RunInEvLoopBlocking([this] {
        process_waiting_commands_timer_->Stop();
        ProcessWaitingCommandsOnStop();
    });
}

void SentinelImpl::SetCommandsBufferingSettings(CommandsBufferingSettings commands_buffering_settings) {
    if (topology_holder_) {
        topology_holder_->SetCommandsBufferingSettings(commands_buffering_settings);
    }
}

void SentinelImpl::SetReplicationMonitoringSettings(const ReplicationMonitoringSettings& monitoring_settings) {
    if (topology_holder_) {
        topology_holder_->SetReplicationMonitoringSettings(monitoring_settings);
    }
}

void SentinelImpl::SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) {
    if (topology_holder_) {
        topology_holder_->SetRetryBudgetSettings(settings);
    }
}

std::unique_ptr<SentinelStatistics> SentinelImpl::GetStatistics(const MetricsSettings& settings) const {
    if (!topology_holder_) {
        return std::make_unique<SentinelStatistics>(settings, SentinelStatisticsInternal{});
    }

    auto stats = std::make_unique<SentinelStatistics>(settings, statistics_internal_);
    topology_holder_->GetStatistics(*stats, settings);
    return stats;
}

void SentinelImpl::EnqueueCommand(const SentinelCommand& command) {
    const std::lock_guard<std::mutex> lock(command_mutex_);
    commands_.push_back(command);
}

size_t SentinelImpl::ShardsCount() const {
    const auto ptr = topology_holder_->GetTopology();
    const auto res = ptr->GetShardsCount();
    UASSERT(res != 0);
    return res;
}

size_t SentinelImpl::GetClusterSlotsCalledCounter() { return ClusterTopologyHolder::GetClusterSlotsCalledCounter(); }

void SentinelImpl::SetConnectionInfo(const std::vector<ConnectionInfoInt>& info_array) {
    topology_holder_->SetConnectionInfo(info_array);
}

void SentinelImpl::UpdatePassword(const Password& password) { topology_holder_->UpdatePassword(password); }

PublishSettings SentinelImpl::GetPublishSettings() {
    if (key_shard_factory_.IsClusterStrategy()) {
        return PublishSettings{kUnknownShard, false, CommandControl::Strategy::kEveryDc};
    } else {
        return PublishSettings{++publish_shard_ % ShardsCount(), true, CommandControl::Strategy::kDefault};
    }
}

}  // namespace storages::redis::impl

// NOLINTEND(clang-analyzer-cplusplus.NewDelete)

USERVER_NAMESPACE_END
