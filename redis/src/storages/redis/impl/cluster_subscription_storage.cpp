#include "cluster_subscription_storage.hpp"

#include <stdexcept>

#include <userver/logging/log.hpp>
#include <userver/utils/rand.hpp>

#include <storages/redis/impl/cluster_topology.hpp>
#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/subscription_rebalance_scheduler.hpp>
#include <userver/storages/redis/impl/reply.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

namespace {

template <typename ChannelInfoType, typename StorageImpl, typename MapType,
          typename CallBackType>
void SubscribeImplImpl(
    StorageImpl& storage_impl, MapType& map,
    const ClusterSubscriptionStorage::ChannelName& channel_name,
    CallBackType cb, const CommandControl& control, SubscriptionId id) {
  const auto& channel = channel_name.channel;
  const std::lock_guard<std::mutex> lock(storage_impl.mutex_);
  auto find_res = map.find(channel);
  if (find_res == map.end()) {
    const size_t selected_shard_idx = ClusterTopology::kUnknownShard;
    find_res = map.insert(
        find_res, std::make_pair(channel, ChannelInfoType(selected_shard_idx)));
    auto& channel_info = find_res->second;
    channel_info.control = control;
    channel_info.active_fsm_count = 1;
    storage_impl.ReadActions(channel_info.info.fsm, channel_name);
  } else {
    auto& channel_info = find_res->second;
    auto& info = channel_info.info;
    channel_info.active_fsm_count = 1;

    shard_subscriber::Event event;
    event.type = shard_subscriber::Event::Type::kSubscribeRequested;
    info.fsm->OnEvent(event);
    storage_impl.ReadActions(info.fsm, channel_name);
  }

  find_res->second.callbacks.insert_or_assign(id, std::move(cb));
}

}  // namespace

ClusterSubscriptionStorage::ClusterSubscriptionStorage(
    const std::shared_ptr<ThreadPools>& thread_pools, size_t shards_count,
    bool /*is_cluster_mode*/)
    : storage_impl_(shards_count, *this), thread_pools_(thread_pools) {
  /// TODO: support multiple shards for ssubscribe
  rebalance_scheduler_ = std::make_unique<SubscriptionRebalanceScheduler>(
      thread_pools->GetSentinelThreadPool(), *this,
      ClusterTopology::kUnknownShard);
}

ClusterSubscriptionStorage::~ClusterSubscriptionStorage() = default;

void ClusterSubscriptionStorage::SetShardsCount(size_t shards_count) {
  storage_impl_.shards_count_ = shards_count;
}

void ClusterSubscriptionStorage::SetSubscribeCallback(CommandCb cb) {
  storage_impl_.subscribe_callback_ = std::move(cb);
}

void ClusterSubscriptionStorage::SetUnsubscribeCallback(CommandCb cb) {
  storage_impl_.unsubscribe_callback_ = std::move(cb);
}

void ClusterSubscriptionStorage::SetShardedSubscribeCallback(
    ShardedCommandCb cb) {
  storage_impl_.sharded_subscribe_callback_ = std::move(cb);
}

void ClusterSubscriptionStorage::SetShardedUnsubscribeCallback(
    ShardedCommandCb cb) {
  storage_impl_.sharded_unsubscribe_callback_ = std::move(cb);
}

SubscriptionToken ClusterSubscriptionStorage::Subscribe(
    const std::string& channel, Sentinel::UserMessageCallback cb,
    CommandControl control) {
  return storage_impl_.Subscribe(channel, std::move(cb), std::move(control));
}

SubscriptionToken ClusterSubscriptionStorage::Psubscribe(
    const std::string& pattern, Sentinel::UserPmessageCallback cb,
    CommandControl control) {
  return storage_impl_.Psubscribe(pattern, std::move(cb), std::move(control));
}

SubscriptionToken ClusterSubscriptionStorage::Ssubscribe(
    const std::string& pattern, Sentinel::UserMessageCallback cb,
    CommandControl control) {
  return storage_impl_.Ssubscribe(pattern, std::move(cb), std::move(control));
}

void ClusterSubscriptionStorage::Unsubscribe(SubscriptionId subscription_id) {
  storage_impl_.Unsubscribe(subscription_id);
}

void ClusterSubscriptionStorage::Stop() {
  {
    const std::unique_lock<std::mutex> lock(storage_impl_.mutex_);
    storage_impl_.callback_map_.clear();
    storage_impl_.pattern_callback_map_.clear();
    storage_impl_.sharded_callback_map_.clear();
  }
  rebalance_scheduler_.reset();
}

RawPubsubClusterStatistics ClusterSubscriptionStorage::GetStatistics() const {
  return storage_impl_.GetStatistics();
}

void ClusterSubscriptionStorage::SetCommandControl(
    const CommandControl& control) {
  storage_impl_.SetCommandControl(control);
}

void ClusterSubscriptionStorage::SetRebalanceMinInterval(
    std::chrono::milliseconds interval) {
  rebalance_scheduler_->SetRebalanceMinInterval(interval);
}

void ClusterSubscriptionStorage::RequestRebalance(size_t /*shard_idx*/,
                                                  ServerWeights weights) {
  rebalance_scheduler_->RequestRebalance(std::move(weights));
}

void ClusterSubscriptionStorage::DoRebalance(size_t shard_idx,
                                             ServerWeights weights) {
  /// Rebalances subscriptions between instances of shard
  if (shard_idx >= storage_impl_.shards_count_ &&
      shard_idx != ClusterTopology::kUnknownShard) {
    throw std::runtime_error("requested rebalance for non-existing shard (" +
                             std::to_string(shard_idx) + " >= " +
                             std::to_string(storage_impl_.shards_count_) + ')');
  }
  storage_impl_.DoRebalance(shard_idx, std::move(weights));
}

void ClusterSubscriptionStorage::SwitchToNonClusterMode() {
  throw std::runtime_error(std::string(__func__) + " Unimplemented yet");
}

void ClusterSubscriptionStorage::SubscribeImpl(const std::string& channel,
                                               Sentinel::UserMessageCallback cb,
                                               CommandControl control,
                                               SubscriptionId id) {
  const ChannelName channel_name(channel, /*pattern=*/false, /*sharded=*/false);
  SubscribeImplImpl<ChannelInfo>(storage_impl_, storage_impl_.callback_map_,
                                 channel_name, std::move(cb), control, id);
}

void ClusterSubscriptionStorage::SsubscribeImpl(
    const std::string& channel, Sentinel::UserMessageCallback cb,
    CommandControl control, SubscriptionId id) {
  const ChannelName channel_name(channel, /*pattern=*/false, /*sharded=*/true);
  SubscribeImplImpl<ChannelInfo>(storage_impl_,
                                 storage_impl_.sharded_callback_map_,
                                 channel_name, std::move(cb), control, id);
}

void ClusterSubscriptionStorage::PsubscribeImpl(
    const std::string& pattern, Sentinel::UserPmessageCallback cb,
    CommandControl control, SubscriptionId id) {
  const ChannelName channel_name(pattern, /*pattern=*/true, /*sharded=*/false);
  SubscribeImplImpl<PChannelInfo>(storage_impl_,
                                  storage_impl_.pattern_callback_map_,
                                  channel_name, std::move(cb), control, id);
}

const std::string& ClusterSubscriptionStorage::GetShardName(
    size_t shard_idx) const {
  return redis::GetShardName(shard_idx);
}

}  // namespace redis

USERVER_NAMESPACE_END
