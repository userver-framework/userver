#include <storages/redis/impl/sentinel.hpp>

#include <memory>

#include <userver/utils/assert.hpp>

#include <storages/redis/impl/cluster_subscription_storage.hpp>
#include <storages/redis/impl/subscription_storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

namespace {

std::unique_ptr<SubscriptionStorageBase> CreateSubscriptionStorage(
    const engine::ev::ThreadControl& thread_control,
    const std::vector<std::string>& shards,
    bool is_cluster_mode
) {
    const auto shards_count = shards.size();
    if (is_cluster_mode) {
        return std::make_unique<ClusterSubscriptionStorage>(thread_control, shards_count);
    }

    return std::make_unique<SubscriptionStorage>(thread_control, shards);
}

}  // namespace

void Sentinel::InitSubscriptionStorage(const std::vector<std::string>& shards) {
    subscription_storage_ = CreateSubscriptionStorage(GetSentinelThreadControl(), shards, IsInClusterMode());
    subscription_storage_->SetCommandControl(GetSubscriptionCommandControl({}));
    subscription_storage_->SetUnsubscribeCallback([this](size_t shard, CommandPtr cmd) {
        AsyncCommand(cmd, false, shard);
    });
    subscription_storage_->SetSubscribeCallback([this](size_t shard, CommandPtr cmd) {
        AsyncCommand(cmd, false, shard);
    });
    subscription_storage_->SetShardedUnsubscribeCallback([this](const std::string& channel, CommandPtr cmd) {
        AsyncCommand(cmd, channel, false);
    });
    subscription_storage_->SetShardedSubscribeCallback([this](const std::string& channel, CommandPtr cmd) {
        AsyncCommand(cmd, channel, false);
    });
}

CommandControl Sentinel::GetSubscriptionCommandControl(const CommandControl& cc) const {
    const auto defaults = subscriber_default_command_control_.Read();
    return secdist_default_subscription_command_control_.MergeWith(*defaults)
        .MergeWith(cc)
        .MergeWith(testsuite_redis_control_);
}

SubscriptionToken Sentinel::Subscribe(
    const std::string& channel,
    const UserMessageCallback& callback,
    CommandControl control
) {
    UINVARIANT(subscription_storage_, "Redis subscriptions are not enabled");
    return subscription_storage_->Subscribe(channel, callback, GetSubscriptionCommandControl(control));
}

SubscriptionToken Sentinel::Psubscribe(
    const std::string& pattern,
    const UserPmessageCallback& callback,
    CommandControl control
) {
    UINVARIANT(subscription_storage_, "Redis subscriptions are not enabled");
    return subscription_storage_->Psubscribe(pattern, callback, GetSubscriptionCommandControl(control));
}

SubscriptionToken Sentinel::Ssubscribe(
    const std::string& channel,
    const UserMessageCallback& callback,
    CommandControl control
) {
    UINVARIANT(subscription_storage_, "Redis subscriptions are not enabled");
    return subscription_storage_->Ssubscribe(channel, callback, GetSubscriptionCommandControl(control));
}

PubsubClusterStatistics Sentinel::GetSubscriberStatistics(const PubsubMetricsSettings& settings) const {
    auto raw = subscription_storage_->GetStatistics();
    PubsubClusterStatistics result(settings, per_channel_stats_enabled_);
    for (auto& shard : raw.by_shard) {
        result.by_shard.emplace(shard.shard_name, std::move(shard));
    }
    return result;
}

void Sentinel::SetSubscriberDefaultCommandControl(const CommandControl& cc) {
    subscriber_default_command_control_.Assign(cc);
    subscription_storage_->SetCommandControl(GetSubscriptionCommandControl({}));
}

void Sentinel::SetRebalanceMinInterval(std::chrono::milliseconds interval) {
    subscription_storage_->SetRebalanceMinInterval(interval);
}

void Sentinel::NotifyInstancesChanged(size_t shard) {
    if (!subscription_storage_) {
        return;
    }
    const auto cc = GetSubscriptionCommandControl({});
    const auto with_master = IsInClusterMode() || cc.allow_reads_from_master.value_or(false);
    subscription_storage_->RequestRebalance(shard, GetAvailableServersWeighted(shard, with_master, cc));
}

void Sentinel::NotifyTopologyChanged(size_t shards_count) {
    if (subscription_storage_) {
        subscription_storage_->SetShardsCount(shards_count);
    }
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
