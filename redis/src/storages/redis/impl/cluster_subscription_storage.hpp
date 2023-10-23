#pragma once
#include "subscription_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

class ClusterSubscriptionStorage : public SubscriptionStorageBase {
 public:
  ClusterSubscriptionStorage(const std::shared_ptr<ThreadPools>& thread_pools,
                             size_t shards_count, bool is_cluster_mode);
  ~ClusterSubscriptionStorage() override;

  void SetSubscribeCallback(CommandCb) override;
  void SetUnsubscribeCallback(CommandCb) override;

  SubscriptionToken Subscribe(const std::string& channel,
                              Sentinel::UserMessageCallback cb,
                              CommandControl control) override;
  SubscriptionToken Psubscribe(const std::string& pattern,
                               Sentinel::UserPmessageCallback cb,
                               CommandControl control) override;

  void Unsubscribe(SubscriptionId subscription_id) override;
  void Stop() override;
  RawPubsubClusterStatistics GetStatistics() const override;
  void SetCommandControl(const CommandControl& control) override;
  void SetRebalanceMinInterval(std::chrono::milliseconds interval) override;
  void RequestRebalance(size_t shard_idx, ServerWeights weights) override;
  void DoRebalance(size_t shard_idx, ServerWeights weights) override;
  void SwitchToNonClusterMode() override;
  void SetShardsCount(size_t shards_count) override;
  const std::string& GetShardName(size_t shard_idx) const override;

  static size_t GetShardByChannel(const std::string& name);

 protected:
  void SubscribeImpl(const std::string& channel,
                     Sentinel::UserMessageCallback cb, CommandControl control,
                     SubscriptionId external_id) override;
  void PsubscribeImpl(const std::string& pattern,
                      Sentinel::UserPmessageCallback cb, CommandControl control,
                      SubscriptionId external_id) override;

 private:
  struct ChannelInfo {
    explicit ChannelInfo(size_t shard_idx) : info(shard_idx) {}

    std::unordered_map<SubscriptionId, Sentinel::UserMessageCallback> callbacks;
    CommandControl control;
    // shard -> Fsm
    ShardChannelInfo info;
    size_t active_fsm_count{0};

    const ShardChannelInfo& GetInfo(size_t /*shard_idx*/) const { return info; }
    ShardChannelInfo& GetInfo(size_t /*shard_idx*/) { return info; }
  };
  struct PChannelInfo {
    explicit PChannelInfo(size_t shard_idx) : info(shard_idx) {}
    std::unordered_map<SubscriptionId, Sentinel::UserPmessageCallback>
        callbacks;
    CommandControl control;
    // shard -> Fsm
    ShardChannelInfo info;
    size_t active_fsm_count{0};

    const ShardChannelInfo& GetInfo(size_t /*shard_idx*/) const { return info; }
    ShardChannelInfo& GetInfo(size_t /*shard_idx*/) { return info; }
  };

  // (p)channel -> subscription_id -> callback
  using CallbackMap = std::unordered_map<std::string, ChannelInfo>;
  using PcallbackMap = std::unordered_map<std::string, PChannelInfo>;

  SubscriptionStorageImpl<CallbackMap, PcallbackMap> storage_impl_;

  std::shared_ptr<ThreadPools> thread_pools_;
  std::unique_ptr<SubscriptionRebalanceScheduler> rebalance_scheduler_;
};

}  // namespace redis

USERVER_NAMESPACE_END
