#include "subscribe_sentinel.hpp"

#include <memory>

#include <userver/logging/log.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include <storages/redis/impl/cluster_subscription_storage.hpp>

#include "sentinel_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {

constexpr std::size_t kSubscriptionDatabaseIndex = 0;

std::unique_ptr<SubscriptionStorageBase> CreateSubscriptionStorage(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const std::vector<std::string>& shards,
    bool is_cluster_mode
) {
    const auto shards_count = shards.size();
    auto shard_names = std::make_shared<const std::vector<std::string>>(shards);
    if (is_cluster_mode) {
        return std::make_unique<ClusterSubscriptionStorage>(thread_pools, shards_count);
    }

    return std::make_unique<SubscriptionStorage>(thread_pools, shards_count, is_cluster_mode, std::move(shard_names));
}

}  // namespace

SubscribeSentinel::SubscribeSentinel(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const std::vector<std::string>& shards,
    const std::vector<ConnectionInfo>& conns,
    std::string shard_group_name,
    dynamic_config::Source dynamic_config_source,
    const std::string& client_name,
    const Password& password,
    ConnectionSecurity connection_security,
    KeyShardFactory key_shard_factory,
    bool is_cluster_mode,
    CommandControl command_control,
    const testsuite::RedisControl& testsuite_redis_control
)
    : Sentinel(
          thread_pools,
          shards,
          conns,
          std::move(shard_group_name),
          client_name,
          password,
          connection_security,
          dynamic_config_source,
          std::move(key_shard_factory),
          command_control,
          testsuite_redis_control,
          kSubscriptionDatabaseIndex

      ),
      storage_(CreateSubscriptionStorage(thread_pools, shards, is_cluster_mode)) {
    InitStorage();
}

SubscribeSentinel::~SubscribeSentinel() {
    storage_->Stop();
    Stop();
}

std::shared_ptr<SubscribeSentinel> SubscribeSentinel::Create(
    const std::shared_ptr<ThreadPools>& thread_pools,
    const secdist::RedisSettings& settings,
    std::string shard_group_name,
    dynamic_config::Source dynamic_config_source,
    const std::string& client_name,
    std::string sharding_strategy,
    const CommandControl& command_control,
    const testsuite::RedisControl& testsuite_redis_control
) {
    const auto& password = settings.password;

    const std::vector<std::string>& shards = settings.shards;
    LOG_DEBUG() << "shards.size() = " << shards.size();
    for (const std::string& shard : shards) LOG_DEBUG() << "shard:  name = " << shard;

    KeyShardFactory keysShardFactory{sharding_strategy};
    auto is_cluster_mode = keysShardFactory.IsClusterStrategy();
    std::vector<ConnectionInfo> conns;
    conns.reserve(settings.sentinels.size());
    LOG_DEBUG() << "sentinels.size() = " << settings.sentinels.size();
    for (const auto& sentinel : settings.sentinels) {
        LOG_DEBUG() << "sentinel:  host = " << sentinel.host << "  port = " << sentinel.port;
        // SENTINEL MASTERS/SLAVES works without auth, sentinel has no AUTH command.
        // CLUSTER SLOTS works after auth only. Masters and slaves used instead of
        // sentinels in cluster mode.
        conns.emplace_back(
            sentinel.host, sentinel.port, (is_cluster_mode ? password : Password("")), false, settings.secure_connection
        );
    }
    LOG_DEBUG() << "redis command_control: " << command_control.ToString();

    if (settings.database_index != kSubscriptionDatabaseIndex) {
        LOG_WARNING() << "`database index` has no effect on susbscriptions. Publishing on database "
                      << settings.database_index << " will be heard by a subscribers on all other databases.";
    }

    auto subscribe_sentinel = std::make_shared<SubscribeSentinel>(
        thread_pools,
        shards,
        conns,
        std::move(shard_group_name),
        dynamic_config_source,
        client_name,
        password,
        settings.secure_connection,
        std::move(keysShardFactory),
        is_cluster_mode,
        command_control,
        testsuite_redis_control
    );
    subscribe_sentinel->Start();
    return subscribe_sentinel;
}

SubscriptionToken SubscribeSentinel::Subscribe(
    const std::string& channel,
    const Sentinel::UserMessageCallback& message_callback,
    CommandControl control
) {
    auto token = storage_->Subscribe(channel, message_callback, GetCommandControl(control));
    return token;
}

SubscriptionToken SubscribeSentinel::Psubscribe(
    const std::string& pattern,
    const Sentinel::UserPmessageCallback& message_callback,
    CommandControl control
) {
    auto token = storage_->Psubscribe(pattern, message_callback, GetCommandControl(control));
    return token;
}

SubscriptionToken SubscribeSentinel::Ssubscribe(
    const std::string& channel,
    const Sentinel::UserMessageCallback& message_callback,
    CommandControl control
) {
    return storage_->Ssubscribe(channel, message_callback, GetCommandControl(control));
}

PubsubClusterStatistics SubscribeSentinel::GetSubscriberStatistics(const PubsubMetricsSettings& settings) const {
    auto raw = storage_->GetStatistics();

    PubsubClusterStatistics result(settings);
    for (auto& shard : raw.by_shard) {
        result.by_shard.emplace(shard.shard_name, std::move(shard));
    }
    return result;
}

void SubscribeSentinel::RebalanceSubscriptions(size_t shard_idx) {
    const auto with_master = IsInClusterMode() || GetCommandControl({}).allow_reads_from_master.value_or(false);
    auto server_weights = GetAvailableServersWeighted(shard_idx, with_master);
    storage_->RequestRebalance(shard_idx, std::move(server_weights));
}

void SubscribeSentinel::SetConfigDefaultCommandControl(const std::shared_ptr<CommandControl>& cc) {
    Sentinel::SetConfigDefaultCommandControl(cc);
    storage_->SetCommandControl(GetCommandControl(*cc));
}

void SubscribeSentinel::SetRebalanceMinInterval(std::chrono::milliseconds interval) {
    storage_->SetRebalanceMinInterval(interval);
}

void SubscribeSentinel::InitStorage() {
    storage_->SetCommandControl(GetCommandControl({}));
    storage_->SetUnsubscribeCallback([this](size_t shard, CommandPtr cmd) { AsyncCommand(cmd, false, shard); });
    storage_->SetSubscribeCallback([this](size_t shard, CommandPtr cmd) { AsyncCommand(cmd, false, shard); });
    storage_->SetShardedUnsubscribeCallback([this](const std::string& channel, CommandPtr cmd) {
        AsyncCommand(cmd, channel, false);
    });
    storage_->SetShardedSubscribeCallback([this](const std::string& channel, CommandPtr cmd) {
        AsyncCommand(cmd, channel, false);
    });
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
