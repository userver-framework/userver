#pragma once

#include <memory>
#include <mutex>
#include <vector>

#include <userver/testsuite/testsuite_support.hpp>

#include <storages/redis/impl/redis_group.hpp>
#include <storages/redis/impl/sentinel.hpp>
#include <storages/redis/impl/subscription_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class SubscribeSentinel : protected Sentinel {
public:
    SubscribeSentinel(
        const std::shared_ptr<ThreadPools>& thread_pools,
        const std::vector<std::string>& shards,
        const std::vector<ConnectionInfo>& conns,
        std::string shard_group_name,
        dynamic_config::Source dynamic_config_source,
        const Credentials& credentials,
        ConnectionSecurity connection_security,
        const testsuite::RedisControl& testsuite_redis_control,
        std::size_t database_index,
        SubscribeSentinelStaticConfig creation_config
    );
    ~SubscribeSentinel() override;

    static std::shared_ptr<SubscribeSentinel> Create(
        const std::shared_ptr<ThreadPools>& thread_pools,
        const USERVER_NAMESPACE::secdist::RedisSettings& settings,
        std::string shard_group_name,
        dynamic_config::Source dynamic_config_source,
        const SubscribeSentinelStaticConfig& creation_config,
        const testsuite::RedisControl& testsuite_redis_control = {}
    );

    SubscriptionToken Subscribe(
        const std::string& channel,
        const Sentinel::UserMessageCallback& message_callback,
        CommandControl control = {}
    );
    SubscriptionToken Psubscribe(
        const std::string& pattern,
        const Sentinel::UserPmessageCallback& message_callback,
        CommandControl control = {}
    );
    SubscriptionToken Ssubscribe(
        const std::string& channel,
        const Sentinel::UserMessageCallback& message_callback,
        CommandControl control = {}
    );

    PubsubClusterStatistics GetSubscriberStatistics(const PubsubMetricsSettings& settings) const;

    void RebalanceSubscriptions(size_t shard_idx);

    void SetConfigDefaultCommandControl(const std::shared_ptr<CommandControl>& cc) override;

    void SetRebalanceMinInterval(std::chrono::milliseconds interval);

    using Sentinel::IsInClusterMode;
    using Sentinel::SetConfigDefaultCommandControl;
    using Sentinel::ShardsCount;
    using Sentinel::WaitConnectedDebug;
    using Sentinel::WaitConnectedOnce;

    void NotifyInstancesChanged(size_t shard) override { RebalanceSubscriptions(shard); }
    void NotifyTopologyChanged(size_t shards_count) override { storage_->SetShardsCount(shards_count); }

private:
    void InitStorage();

    std::unique_ptr<SubscriptionStorageBase> storage_;
    bool per_channel_stats_enabled_{true};
};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
