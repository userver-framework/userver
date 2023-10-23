#pragma once
#include "subscription_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

/// Class stores actual implementation of SubscriptionStorage and allow to
/// switch it in runtime. Must be removed and replaced with
/// ClusterSubscriptionStorage after TAXICOMMON-6018
class SubscriptionStorageSwitcher : public SubscriptionStorageBase {
 public:
  SubscriptionStorageSwitcher(
      const std::shared_ptr<ThreadPools>& thread_pools, size_t shards_count,
      bool is_cluster_mode,
      std::shared_ptr<const std::vector<std::string>> shard_names,
      dynamic_config::Source dynamic_config_source);

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

  void SubscribeImpl(const std::string&, Sentinel::UserMessageCallback,
                     CommandControl, SubscriptionId) override{};
  void PsubscribeImpl(const std::string&, Sentinel::UserPmessageCallback,
                      CommandControl, SubscriptionId) override {}

  void ChangeSubscriptionStorageImpl(bool autotopology, size_t shards_count,
                                     CommandCb old_sentinel_callback,
                                     CommandCb new_sentinel_callback);

 private:
  struct Params {
    std::shared_ptr<ThreadPools> thread_pools;
    size_t shards_count = 0;
    std::shared_ptr<const std::vector<std::string>> shard_names;
    bool is_cluster_mode = false;
  };

  utils::SwappingSmart<SubscriptionStorageBase> impl_;
  Params params_;

  /// This maps hold subscription_ids and callbacks to be able to resubscribe
  /// actual storages when switching implementations
  template <typename Callback>
  struct SubscribeInfo {
    SubscribeInfo(Callback callback, CommandControl control)
        : callback(std::move(callback)), control(std::move(control)) {}

    Callback callback;
    CommandControl control;
  };
  using SubscribeMap = std::unordered_map<
      std::string,
      std::unordered_map<SubscriptionId,
                         SubscribeInfo<Sentinel::UserMessageCallback>>>;
  using PsubscribeMap = std::unordered_map<
      std::string,
      std::unordered_map<SubscriptionId,
                         SubscribeInfo<Sentinel::UserPmessageCallback>>>;
  /// Mutex protects next_subscription_id_, subscribe_map_ and psubscribe_map_
  mutable std::mutex mutex_;
  SubscriptionId next_subscription_id_{1};
  SubscribeMap subscribe_map_;
  PsubscribeMap psubscribe_map_;
};

}  // namespace redis

USERVER_NAMESPACE_END
