#pragma once

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <storages/redis/impl/sentinel.hpp>
#include <userver/utils/rand.hpp>

#include "redis.hpp"
#include "shard_subscription_fsm.hpp"
#include "subscription_statistics.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

using SubscriptionId = size_t;

class SubscriptionStorageBase;
class SubscriptionRebalanceScheduler;

class SubscriptionToken {
 public:
  SubscriptionToken() = default;
  SubscriptionToken(std::weak_ptr<SubscriptionStorageBase> storage,
                    SubscriptionId subscription_id);
  SubscriptionToken(SubscriptionToken&& token) noexcept;
  SubscriptionToken(const SubscriptionToken& token) = delete;

  SubscriptionToken& operator=(SubscriptionToken&& token) noexcept;

  bool Subscribed() const { return subscription_id_ != 0; }

  ~SubscriptionToken();

  void Unsubscribe();

 private:
  std::weak_ptr<SubscriptionStorageBase> storage_;
  SubscriptionId subscription_id_{0};
};

class SubscriptionStorageBase
    : public std::enable_shared_from_this<SubscriptionStorageBase> {
 public:
  using ServerWeights = std::unordered_map<ServerId, size_t, ServerIdHasher>;
  using CommandCb = std::function<void(size_t shard, CommandPtr command)>;
  using ShardedCommandCb =
      std::function<void(const std::string& channel, CommandPtr command)>;
  struct ChannelName {
    ChannelName() = default;
    ChannelName(std::string channel, bool pattern, bool sharded)
        : channel(std::move(channel)), pattern(pattern), sharded(sharded) {}

    std::string channel;
    bool pattern{false};
    bool sharded{false};
  };

  virtual ~SubscriptionStorageBase() = default;

  virtual void SetSubscribeCallback(CommandCb) = 0;
  virtual void SetUnsubscribeCallback(CommandCb) = 0;
  virtual void SetShardedSubscribeCallback(ShardedCommandCb) = 0;
  virtual void SetShardedUnsubscribeCallback(ShardedCommandCb) = 0;

  virtual SubscriptionToken Subscribe(const std::string& channel,
                                      Sentinel::UserMessageCallback cb,
                                      CommandControl control) = 0;
  virtual SubscriptionToken Psubscribe(const std::string& pattern,
                                       Sentinel::UserPmessageCallback cb,
                                       CommandControl control) = 0;
  virtual SubscriptionToken Ssubscribe(const std::string& pattern,
                                       Sentinel::UserMessageCallback cb,
                                       CommandControl control) = 0;

  virtual void Unsubscribe(SubscriptionId subscription_id) = 0;

  virtual void Stop() = 0;

  virtual RawPubsubClusterStatistics GetStatistics() const = 0;

  virtual void SetCommandControl(const CommandControl& control) = 0;
  virtual void SetRebalanceMinInterval(std::chrono::milliseconds interval) = 0;

  virtual void RequestRebalance(size_t shard_idx, ServerWeights weights) = 0;

  virtual void DoRebalance(size_t shard_idx, ServerWeights weights) = 0;

  virtual void SwitchToNonClusterMode() = 0;
  virtual void SetShardsCount(size_t /*shards_count*/) = 0;

  virtual const std::string& GetShardName(size_t shard_idx) const = 0;

  ///{ Methods for implementing SubscriptionStorageSwitcher.
  ///  Do NOT use them anywhere else. This methods will be removed after
  ///  experiment
  virtual void SubscribeImpl(const std::string& channel,
                             Sentinel::UserMessageCallback cb,
                             CommandControl control,
                             SubscriptionId external_id) = 0;
  virtual void SsubscribeImpl(const std::string& channel,
                              Sentinel::UserMessageCallback cb,
                              CommandControl control,
                              SubscriptionId external_id) = 0;
  virtual void PsubscribeImpl(const std::string& pattern,
                              Sentinel::UserPmessageCallback cb,
                              CommandControl control,
                              SubscriptionId external_id) = 0;
  ///}

  using FsmPtr = std::shared_ptr<shard_subscriber::Fsm>;

  struct ShardChannelInfo {
    explicit ShardChannelInfo(size_t shard_idx, bool fake = false)
        : fsm(fake ? nullptr
                   : std::make_shared<shard_subscriber::Fsm>(shard_idx)) {}

    FsmPtr fsm;
    PubsubChannelStatistics statistics;

    PubsubChannelStatistics GetStatistics() const {
      if (!fsm) return {};
      PubsubChannelStatistics stats(statistics);
      stats.server_id = fsm->GetCurrentServerId();
      stats.subscription_timestamp = fsm->GetCurrentServerTimePoint();
      return stats;
    }

    void AccountMessage(ServerId server_id, size_t message_size);
    void AccountDiscardedByOverflow(size_t discarded);
  };

 protected:
  using MessageCallback = std::function<void(
      ServerId, const std::string& channel, const std::string& message)>;
  using PmessageCallback = std::function<void(
      ServerId, const std::string& pattern, const std::string& channel,
      const std::string& message)>;

  using SubscribedCallbackOutcome = Sentinel::Outcome;

  struct RebalanceState {
    RebalanceState(size_t shard_idx, ServerWeights weights);

    const size_t shard_idx;
    ServerWeights weights;
    size_t sum_weights{0};
    size_t total_connections{0};
    // Use std::map instead of std::unordered_map because we rely on fact that
    // iterators are never invalidated on inserts in RebalanceMoveSubscriptions
    // method
    std::map<ServerId, std::vector<std::pair<ChannelName, FsmPtr>>>
        subscriptions_by_server;
    std::map<ServerId, size_t> need_subscription_count;
  };

  template <typename CallbackMap, typename PcallbackMap>
  class SubscriptionStorageImpl {
   public:
    SubscriptionStorageImpl(size_t shards_count,
                            SubscriptionStorageBase& implemented)
        : shards_count_(shards_count), implemented_(implemented) {}
    void Unsubscribe(SubscriptionId subscription_id);

    void ReadActions(FsmPtr fsm, const ChannelName& channel_name);
    void HandleChannelAction(FsmPtr fsm, shard_subscriber::Action action,
                             const ChannelName& channel_name);
    void HandleServerStateChanged(
        const std::shared_ptr<shard_subscriber::Fsm>& fsm,
        const ChannelName& channel_name, ServerId server_id,
        shard_subscriber::Event::Type event_type);

    template <class Map>
    void DeleteChannel(Map& callback_map, const ChannelName& channel_name,
                       const FsmPtr& fsm);

    template <class Map>
    bool DoUnsubscribe(Map& callback_map, SubscriptionId id, bool sharded);

    CommandPtr PrepareUnsubscribeCommand(const ChannelName& channel_name);

    enum class SubscriberEvent {
      kSubscriberConnected,
      kSubscriberDisconnected
    };
    static shard_subscriber::Event::Type EventTypeFromSubscriberEvent(
        SubscriberEvent event);

    using SubscribeCb = std::function<void(ServerId, SubscriberEvent event)>;
    CommandPtr PrepareSubscribeCommand(const ChannelName& channel_name,
                                       SubscribeCb cb, size_t shard_idx);

    void OnMessage(ServerId server_id, const std::string& channel,
                   const std::string& message, size_t shard_idx);
    void OnPmessage(ServerId server_id, const std::string& pattern,
                    const std::string& channel, const std::string& message,
                    size_t shard_idx);
    void OnSmessage(ServerId server_id, const std::string& channel,
                    const std::string& message, size_t shard_idx);
    size_t GetChannelsCountApprox() const;
    PubsubShardStatistics GetShardStatistics(size_t shard_idx) const;
    RawPubsubClusterStatistics GetStatistics() const;

    template <typename Map>
    static void RebalanceGatherSubscriptions(RebalanceState& state,
                                             Map& callback_map, bool pattern,
                                             bool sharded);
    void RebalanceCalculateNeedCount(RebalanceState& state);

    void RebalanceMoveSubscriptions(RebalanceState& state);

    void SetCommandControl(const CommandControl& control);
    void DoRebalance(size_t shard_idx, ServerWeights weights);
    SubscriptionToken Subscribe(const std::string& channel,
                                Sentinel::UserMessageCallback cb,
                                CommandControl control);
    SubscriptionToken Ssubscribe(const std::string& channel,
                                 Sentinel::UserMessageCallback cb,
                                 CommandControl control);
    SubscriptionToken Psubscribe(const std::string& channel,
                                 Sentinel::UserPmessageCallback cb,
                                 CommandControl control);
    SubscriptionId GetNextSubscriptionId();

    mutable std::mutex mutex_;
    CommandCb subscribe_callback_;
    CommandCb unsubscribe_callback_;
    ShardedCommandCb sharded_subscribe_callback_;
    ShardedCommandCb sharded_unsubscribe_callback_;
    CallbackMap callback_map_;
    PcallbackMap pattern_callback_map_;
    CallbackMap sharded_callback_map_;
    CommandControl common_command_control_;
    size_t shards_count_{0};
    SubscriptionStorageBase& implemented_;
    SubscriptionId next_subscription_id_{1};
  };
};

class SubscriptionStorage : public SubscriptionStorageBase {
 public:
  SubscriptionStorage(
      const std::shared_ptr<ThreadPools>& thread_pools, size_t shards_count,
      bool is_cluster_mode,
      std::shared_ptr<const std::vector<std::string>> shard_names);
  SubscriptionStorage(
      size_t shards_count, bool is_cluster_mode,
      std::shared_ptr<const std::vector<std::string>> shard_names);
  ~SubscriptionStorage() override;

  void SetSubscribeCallback(CommandCb) override;
  void SetUnsubscribeCallback(CommandCb) override;
  void SetShardedSubscribeCallback(ShardedCommandCb) override;
  void SetShardedUnsubscribeCallback(ShardedCommandCb) override;

  SubscriptionToken Subscribe(const std::string& channel,
                              Sentinel::UserMessageCallback cb,
                              CommandControl control) override;
  SubscriptionToken Ssubscribe(const std::string& channel,
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
  void SetShardsCount(size_t /*shards_count*/) override {}
  const std::string& GetShardName(size_t shard_idx) const override;

  void SubscribeImpl(const std::string& channel,
                     Sentinel::UserMessageCallback cb, CommandControl control,
                     SubscriptionId external_id) override;
  void SsubscribeImpl(const std::string& channel,
                      Sentinel::UserMessageCallback cb, CommandControl control,
                      SubscriptionId external_id) override;
  void PsubscribeImpl(const std::string& pattern,
                      Sentinel::UserPmessageCallback cb, CommandControl control,
                      SubscriptionId external_id) override;

 private:
  /* We could use Fsm per shard (single Fsm for all channels), but in
   * this case it would be hard to create new subsciptions for shards
   * with Fsms in transition states (e.g. previous subscription
   * requests are sent, we're waiting for the reply). So use
   * independent Fsms for distinct channels.
   */
  struct ChannelInfo {
    std::map<SubscriptionId, Sentinel::UserMessageCallback> callbacks;
    CommandControl control;
    // shard -> Fsm
    std::vector<ShardChannelInfo> info;
    size_t active_fsm_count{0};

    const ShardChannelInfo& GetInfo(size_t shard_idx) const {
      return info[shard_idx];
    }
    ShardChannelInfo& GetInfo(size_t shard_idx) { return info[shard_idx]; }
  };
  struct PChannelInfo {
    std::map<SubscriptionId, Sentinel::UserPmessageCallback> callbacks;
    CommandControl control;
    // shard -> Fsm
    std::vector<ShardChannelInfo> info;
    size_t active_fsm_count{0};

    const ShardChannelInfo& GetInfo(size_t shard_idx) const {
      return info[shard_idx];
    }
    ShardChannelInfo& GetInfo(size_t shard_idx) { return info[shard_idx]; }
  };

  // (p)channel -> subscription_id -> callback
  using CallbackMap = std::map<std::string, ChannelInfo>;
  using PcallbackMap = std::map<std::string, PChannelInfo>;

  SubscriptionStorageImpl<CallbackMap, PcallbackMap> storage_impl_;
  std::shared_ptr<const std::vector<std::string>> shard_names_;
  bool is_cluster_mode_;

  size_t shard_rotate_counter_;

  std::vector<std::unique_ptr<SubscriptionRebalanceScheduler>>
      rebalance_schedulers_;
};

}  // namespace redis

USERVER_NAMESPACE_END
