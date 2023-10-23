#include "subscription_storage_switcher.hpp"

#include <memory>
#include <utility>

#include <userver/dynamic_config/value.hpp>
#include <userver/logging/log.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/utils/impl/userver_experiments.hpp>

#include "cluster_subscription_storage.hpp"
#include "storages/redis/dynamic_config.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {
SubscriptionStorageSwitcher::SubscriptionStorageSwitcher(
    const std::shared_ptr<ThreadPools>& thread_pools, size_t shards_count,
    bool is_cluster_mode,
    std::shared_ptr<const std::vector<std::string>> shard_names,
    dynamic_config::Source dynamic_config_source)
    : params_{thread_pools, shards_count, std::move(shard_names),
              is_cluster_mode} {
  const auto config_snapshot = dynamic_config_source.GetSnapshot();
  const auto enabled_by_config = config_snapshot[kRedisAutoTopologyEnabled];
  const auto enabled_by_exp =
      utils::impl::kRedisClusterAutoTopologyExperiment.IsEnabled();

  if (is_cluster_mode && enabled_by_config && enabled_by_exp) {
    impl_.Set(std::make_shared<ClusterSubscriptionStorage>(
        thread_pools, shards_count, is_cluster_mode));
  } else {
    impl_.Set(std::make_shared<SubscriptionStorage>(
        thread_pools, shards_count, is_cluster_mode, params_.shard_names));
  }
}

void SubscriptionStorageSwitcher::SetSubscribeCallback(CommandCb callback) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SetSubscribeCallback(std::move(callback));
}

void SubscriptionStorageSwitcher::SetUnsubscribeCallback(CommandCb callback) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SetUnsubscribeCallback(std::move(callback));
}

SubscriptionToken SubscriptionStorageSwitcher::Subscribe(
    const std::string& channel, Sentinel::UserMessageCallback cb,
    CommandControl control) {
  auto impl = impl_.Get();
  UASSERT(impl);

  const std::lock_guard<std::mutex> lock(mutex_);
  auto id = next_subscription_id_++;
  SubscriptionToken token(shared_from_this(), id);
  LOG_DEBUG() << "Subscribe on channel=" << channel << " id=" << id;
  subscribe_map_[channel].emplace(id, SubscribeInfo(cb, control));

  impl->SubscribeImpl(channel, std::move(cb), std::move(control), id);
  return token;
}

SubscriptionToken SubscriptionStorageSwitcher::Psubscribe(
    const std::string& pattern, Sentinel::UserPmessageCallback cb,
    CommandControl control) {
  auto impl = impl_.Get();
  UASSERT(impl);

  const std::lock_guard<std::mutex> lock(mutex_);
  auto id = next_subscription_id_++;
  SubscriptionToken token(shared_from_this(), id);
  LOG_DEBUG() << "Subscribe on channel=" << pattern << " id=" << id;
  psubscribe_map_[pattern].emplace(id, SubscribeInfo(cb, control));

  impl->PsubscribeImpl(pattern, std::move(cb), std::move(control), id);
  return token;
}

void SubscriptionStorageSwitcher::Unsubscribe(SubscriptionId subscription_id) {
  auto impl = impl_.Get();
  UASSERT(impl);

  const std::lock_guard<std::mutex> lock(mutex_);
  impl->Unsubscribe(std::move(subscription_id));
  for (auto& [_, v] : subscribe_map_) {
    if (v.erase(subscription_id)) {
      return;
    }
  }
  for (auto& [_, v] : psubscribe_map_) {
    if (v.erase(subscription_id)) {
      return;
    }
  }
}

void SubscriptionStorageSwitcher::Stop() {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->Stop();
}

RawPubsubClusterStatistics SubscriptionStorageSwitcher::GetStatistics() const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->GetStatistics();
}

void SubscriptionStorageSwitcher::SetCommandControl(
    const CommandControl& control) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SetCommandControl(control);
}

void SubscriptionStorageSwitcher::SetRebalanceMinInterval(
    std::chrono::milliseconds interval) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SetRebalanceMinInterval(interval);
}

void SubscriptionStorageSwitcher::RequestRebalance(size_t shard_idx,
                                                   ServerWeights weights) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->RequestRebalance(shard_idx, std::move(weights));
}

void SubscriptionStorageSwitcher::DoRebalance(size_t shard_idx,
                                              ServerWeights weights) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->DoRebalance(shard_idx, std::move(weights));
}

void SubscriptionStorageSwitcher::SwitchToNonClusterMode() {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SwitchToNonClusterMode();
}

void SubscriptionStorageSwitcher::SetShardsCount(size_t shards_count) {
  auto impl = impl_.Get();
  UASSERT(impl);
  impl->SetShardsCount(shards_count);
}

void SubscriptionStorageSwitcher::ChangeSubscriptionStorageImpl(
    bool autotopology, size_t shards_count, CommandCb old_sentinel_callback,
    CommandCb new_sentinel_callback) {
  auto impl = impl_.Get();
  UASSERT(impl);

  // Create new storage
  std::shared_ptr<SubscriptionStorageBase> ptr;
  if (autotopology) {
    ptr = std::make_shared<ClusterSubscriptionStorage>(
        params_.thread_pools, shards_count, params_.is_cluster_mode);
  } else {
    ptr = std::make_shared<SubscriptionStorage>(
        params_.thread_pools, params_.shards_count, params_.is_cluster_mode,
        params_.shard_names);
  }
  ptr->SetSubscribeCallback(new_sentinel_callback);
  ptr->SetUnsubscribeCallback(new_sentinel_callback);

  // unsubscribe old storage
  // subscribe to new storage
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    impl->SetUnsubscribeCallback(old_sentinel_callback);

    for (const auto& [channel, callbacks] : subscribe_map_) {
      for (const auto& [id, value] : callbacks) {
        impl->Unsubscribe(id);
        ptr->SubscribeImpl(channel, value.callback, value.control, id);
      }
    }
    for (const auto& [pattern, callbacks] : psubscribe_map_) {
      for (const auto& [id, value] : callbacks) {
        impl->Unsubscribe(id);
        ptr->PsubscribeImpl(pattern, value.callback, value.control, id);
      }
    }

    impl_.Set(std::move(ptr));
  }
}

const std::string& SubscriptionStorageSwitcher::GetShardName(
    size_t shard_idx) const {
  auto impl = impl_.Get();
  UASSERT(impl);
  return impl->GetShardName(shard_idx);
}

}  // namespace redis

USERVER_NAMESPACE_END
