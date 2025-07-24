#include <storages/redis/impl/sentinel.hpp>

#include <memory>
#include <stdexcept>

#include <engine/ev/thread_control.hpp>

#include <userver/dynamic_config/value.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/exception.hpp>
#include <userver/storages/redis/reply.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <storages/redis/impl/cluster_sentinel_impl.hpp>
#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/redis.hpp>
#include <storages/redis/impl/sentinel_impl.hpp>
#include <storages/redis/impl/subscribe_sentinel.hpp>

#include "command_control_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {
namespace {

void ThrowIfCancelled() {
    if (engine::current_task::IsTaskProcessorThread() && engine::current_task::ShouldCancel()) {
        throw RequestCancelledException("Failed to make redis request due to task cancellation");
    }
}

void OnSubscribeImpl(
    std::string_view message_type,
    const Sentinel::MessageCallback& message_callback,
    std::string_view subscribe_type,
    const Sentinel::SubscribeCallback& subscribe_callback,
    std::string_view unsubscribe_type,
    const Sentinel::UnsubscribeCallback& unsubscribe_callback,
    const ReplyPtr& reply
) {
    if (!reply->data.IsArray()) return;
    const auto& reply_array = reply->data.GetArray();
    if (reply_array.size() != 3 || !reply_array[0].IsString()) return;
    if (!strcasecmp(reply_array[0].GetString().c_str(), subscribe_type.data())) {
        subscribe_callback(reply->server_id, reply_array[1].GetString(), reply_array[2].GetInt());
    } else if (!strcasecmp(reply_array[0].GetString().c_str(), unsubscribe_type.data())) {
        unsubscribe_callback(reply->server_id, reply_array[1].GetString(), reply_array[2].GetInt());
    } else if (!strcasecmp(reply_array[0].GetString().c_str(), message_type.data())) {
        message_callback(reply->server_id, reply_array[1].GetString(), reply_array[2].GetString());
    }
}

}  // namespace

Sentinel::Sentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns,
    std::string shard_group_name,
    const std::string& client_name,
    const Password& password,
    ConnectionSecurity connection_security,
    dynamic_config::Source dynamic_config_source,
    KeyShardFactory key_shard_factory,
    CommandControl command_control,
    const testsuite::RedisControl& testsuite_redis_control,
    std::size_t database_index
)
    : shard_group_name_(shard_group_name),
      thread_pools_(thread_pools),
      secdist_default_command_control_(command_control),
      testsuite_redis_control_(testsuite_redis_control),
      is_in_cluster_mode_(key_shard_factory.IsClusterStrategy()) {
    config_default_command_control_.Set(std::make_shared<CommandControl>(secdist_default_command_control_));

    if (!thread_pools_) {
        throw std::runtime_error("can't create Sentinel with empty thread_pools");
    }
    sentinel_thread_control_ =
        std::make_unique<engine::ev::ThreadControl>(thread_pools_->GetSentinelThreadPool().NextThread());

    UINVARIANT(
        !key_shard_factory.IsClusterStrategy() || database_index == 0,
        "Database index other than 0 now supported in cluster and standalone modes"
    );
    sentinel_thread_control_->RunInEvLoopBlocking([&]() {
        if (IsInClusterMode()) {
            impl_ = std::make_unique<ClusterSentinelImpl>(
                *sentinel_thread_control_,
                thread_pools_->GetRedisThreadPool(),
                *this,
                shards,
                conns,
                std::move(shard_group_name),
                client_name,
                password,
                connection_security,
                key_shard_factory(shards.size()),
                dynamic_config_source
            );
        } else {
            impl_ = std::make_unique<SentinelImpl>(
                *sentinel_thread_control_,
                thread_pools_->GetRedisThreadPool(),
                *this,
                shards,
                conns,
                std::move(shard_group_name),
                client_name,
                password,
                connection_security,
                key_shard_factory(shards.size()),
                dynamic_config_source,
                database_index
            );
        }
    });
}

Sentinel::~Sentinel() { Stop(); }

void Sentinel::Start() {
    sentinel_thread_control_->RunInEvLoopBlocking([this] { impl_->Start(); });
}

void Sentinel::WaitConnectedDebug(bool allow_empty_slaves) { impl_->WaitConnectedDebug(allow_empty_slaves); }

void Sentinel::WaitConnectedOnce(RedisWaitConnected wait_connected) {
    impl_->WaitConnectedOnce(wait_connected.MergeWith(testsuite_redis_control_));
}

void Sentinel::ForceUpdateHosts() { impl_->ForceUpdateHosts(); }

std::shared_ptr<Sentinel> Sentinel::CreateSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings,
    std::string shard_group_name,
    dynamic_config::Source dynamic_config_source,
    const std::string& client_name,
    KeyShardFactory key_shard_factory,
    const CommandControl& command_control,
    const testsuite::RedisControl& testsuite_redis_control
) {
    const auto& password = settings.password;

    const std::vector<std::string>& shards = settings.shards;
    LOG_DEBUG() << "shards.size() = " << shards.size();
    for (const std::string& shard : shards) LOG_DEBUG() << "shard:  name = " << shard;

    std::vector<redis::ConnectionInfo> conns;
    conns.reserve(settings.sentinels.size());
    LOG_DEBUG() << "sentinels.size() = " << settings.sentinels.size();
    for (const auto& sentinel : settings.sentinels) {
        LOG_DEBUG() << "sentinel:  host = " << sentinel.host << "  port = " << sentinel.port;
        // SENTINEL MASTERS/SLAVES works without auth, sentinel has no AUTH command.
        // CLUSTER SLOTS works after auth only. Masters and slaves used instead of
        // sentinels in cluster mode.
        conns.emplace_back(
            sentinel.host,
            sentinel.port,
            (key_shard_factory.IsClusterStrategy() ? password : Password("")),
            false,
            settings.secure_connection
        );
    }

    LOG_DEBUG() << "redis command_control:" << command_control.ToString();
    std::shared_ptr<storages::redis::impl::Sentinel> client;
    if (!shards.empty() && !conns.empty()) {
        client = std::make_shared<storages::redis::impl::Sentinel>(
            thread_pools,
            shards,
            conns,
            std::move(shard_group_name),
            client_name,
            password,
            settings.secure_connection,
            dynamic_config_source,
            std::move(key_shard_factory),
            command_control,
            testsuite_redis_control,
            settings.database_index
        );
        client->Start();
    }

    return client;
}

void Sentinel::Stop() noexcept {
    impl_.reset();
    UASSERT(!impl_);
}

std::unordered_map<ServerId, size_t, ServerIdHasher>
Sentinel::GetAvailableServersWeighted(size_t shard_idx, bool with_master, const CommandControl& cc) const {
    return impl_->GetAvailableServersWeighted(shard_idx, with_master, GetCommandControl(cc));
}

void Sentinel::AsyncCommand(CommandPtr command, bool master, size_t shard) {
    if (!impl_) return;
    ThrowIfCancelled();

    if (CommandControlImpl{command->control}.force_request_to_master) {
        master = true;
    }
    if (command->control.force_shard_idx) {
        if (IsInClusterMode()) throw InvalidArgumentException("force_shard_idx is not supported in RedisCluster mode");
        if (shard != *command->control.force_shard_idx)

            throw InvalidArgumentException(
                "shard index in argument differs from force_shard_idx in "
                "command_control (" +
                std::to_string(shard) + " != " + std::to_string(*command->control.force_shard_idx) + ')'
            );
    }
    CheckShardIdx(shard);
    try {
        impl_->AsyncCommand(
            {command, master, shard, std::chrono::steady_clock::now()}, SentinelImplBase::kDefaultPrevInstanceIdx
        );
    } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in " << __func__ << " '" << ex.what() << "'";
    }
}

void Sentinel::AsyncCommand(CommandPtr command, const std::string& key, bool master) {
    if (!impl_) return;
    ThrowIfCancelled();

    if (CommandControlImpl{command->control}.force_request_to_master) {
        master = true;
    }
    size_t shard = 0;
    if (command->control.force_shard_idx) {
        if (IsInClusterMode()) throw InvalidArgumentException("force_shard_idx is not supported in RedisCluster mode");
        shard = *command->control.force_shard_idx;
    } else {
        shard = impl_->ShardByKey(key);
    }

    CheckShardIdx(shard);
    try {
        impl_->AsyncCommand(
            {command, master, shard, std::chrono::steady_clock::now()}, SentinelImplBase::kDefaultPrevInstanceIdx
        );
    } catch (const std::exception& ex) {
        LOG_WARNING() << "exception in " << __func__ << " '" << ex.what() << "'";
    }
}

std::string Sentinel::CreateTmpKey(const std::string& key, std::string prefix) {
    size_t key_start = 0;
    size_t key_len = 0;
    GetRedisKey(key, &key_start, &key_len);

    std::string tmp_key{std::move(prefix)};
    if (key_start == 0) {
        tmp_key.push_back('{');
        tmp_key.append(key);
        tmp_key.push_back('}');
    } else {
        tmp_key.append(key);
    }
    return tmp_key;
}

size_t Sentinel::ShardByKey(const std::string& key) const { return impl_->ShardByKey(key); }

size_t Sentinel::ShardsCount() const { return impl_->ShardsCount(); }

void Sentinel::CheckShardIdx(size_t shard_idx) const { CheckShardIdx(shard_idx, ShardsCount()); }

void Sentinel::CheckShardIdx(size_t shard_idx, size_t shard_count) {
    if (shard_idx >= shard_count && shard_idx != ClusterSentinelImpl::kUnknownShard) {
        throw InvalidArgumentException(
            "invalid shard (" + std::to_string(shard_idx) + " >= " + std::to_string(shard_count) + ')'
        );
    }
}

SentinelStatistics Sentinel::GetStatistics(const MetricsSettings& settings) const {
    return impl_->GetStatistics(settings);
}

void Sentinel::SetCommandsBufferingSettings(CommandsBufferingSettings commands_buffering_settings) {
    return impl_->SetCommandsBufferingSettings(commands_buffering_settings);
}

void Sentinel::SetReplicationMonitoringSettings(const ReplicationMonitoringSettings& replication_monitoring_settings) {
    impl_->SetReplicationMonitoringSettings(replication_monitoring_settings);
}

void Sentinel::SetRetryBudgetSettings(const utils::RetryBudgetSettings& settings) {
    impl_->SetRetryBudgetSettings(settings);
}

void Sentinel::OnSsubscribeReply(
    const MessageCallback& message_callback,
    const SubscribeCallback& subscribe_callback,
    const UnsubscribeCallback& unsubscribe_callback,
    ReplyPtr reply
) {
    OnSubscribeImpl(
        "SMESSAGE", message_callback, "SSUBSCRIBE", subscribe_callback, "SUNSUBSCRIBE", unsubscribe_callback, reply
    );
}

void Sentinel::OnSubscribeReply(
    const MessageCallback& message_callback,
    const SubscribeCallback& subscribe_callback,
    const UnsubscribeCallback& unsubscribe_callback,
    ReplyPtr reply
) {
    OnSubscribeImpl(
        "MESSAGE", message_callback, "SUBSCRIBE", subscribe_callback, "UNSUBSCRIBE", unsubscribe_callback, reply
    );
}

void Sentinel::OnPsubscribeReply(
    const PmessageCallback& pmessage_callback,
    const SubscribeCallback& subscribe_callback,
    const UnsubscribeCallback& unsubscribe_callback,
    ReplyPtr reply
) {
    if (!reply->data.IsArray()) return;
    const auto& reply_array = reply->data.GetArray();
    if (!reply_array[0].IsString()) return;
    if (!strcasecmp(reply_array[0].GetString().c_str(), "PSUBSCRIBE")) {
        if (reply_array.size() == 3)
            subscribe_callback(reply->server_id, reply_array[1].GetString(), reply_array[2].GetInt());
    } else if (!strcasecmp(reply_array[0].GetString().c_str(), "PUNSUBSCRIBE")) {
        if (reply_array.size() == 3)
            unsubscribe_callback(reply->server_id, reply_array[1].GetString(), reply_array[2].GetInt());
    } else if (!strcasecmp(reply_array[0].GetString().c_str(), "PMESSAGE")) {
        if (reply_array.size() == 4)
            pmessage_callback(
                reply->server_id, reply_array[1].GetString(), reply_array[2].GetString(), reply_array[3].GetString()
            );
    }
}

CommandControl Sentinel::GetCommandControl(const CommandControl& cc) const {
    return secdist_default_command_control_.MergeWith(*config_default_command_control_.Get())
        .MergeWith(cc)
        .MergeWith(testsuite_redis_control_);
}

PublishSettings Sentinel::GetPublishSettings() const { return impl_->GetPublishSettings(); }

void Sentinel::SetConfigDefaultCommandControl(const std::shared_ptr<CommandControl>& cc) {
    config_default_command_control_.Set(cc);
}

const std::string& Sentinel::ShardGroupName() const { return shard_group_name_; }

void Sentinel::UpdatePassword(const Password& password) { impl_->UpdatePassword(password); }

void Sentinel::SetConnectionInfo(std::vector<ConnectionInfo> info_array) {
    std::vector<ConnectionInfoInt> cii;
    cii.reserve(info_array.size());
    for (const auto& ci : info_array) cii.emplace_back(ci);

    impl_->SetConnectionInfo(cii);
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
