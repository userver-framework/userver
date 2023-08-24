#pragma once

#include <map>
#include <mutex>
#include <string>

#include <storages/redis/impl/sentinel.hpp>

#include "redis.hpp"
#include "shard_subscription_fsm.hpp"
#include "subscription_statistics.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

using SubscriptionId = size_t;

class SubscriptionStorage;
class SubscriptionRebalanceScheduler;

class SubscriptionToken {
 public:
  SubscriptionToken() = default;
  SubscriptionToken(std::weak_ptr<SubscriptionStorage> storage,
                    SubscriptionId subscription_id);
  SubscriptionToken(SubscriptionToken&& token) noexcept;
  SubscriptionToken(const SubscriptionToken& token) = delete;

  SubscriptionToken& operator=(SubscriptionToken&& token) noexcept;

  bool Subscribed() const { return subscription_id_ != 0; }

  ~SubscriptionToken();

  void Unsubscribe();

 private:
  std::weak_ptr<SubscriptionStorage> storage_;
  SubscriptionId subscription_id_{0};
};

class SubscriptionStorage
    : public std::enable_shared_from_this<SubscriptionStorage> {
 public:
  SubscriptionStorage(const std::shared_ptr<ThreadPools>& thread_pools,
                      size_t shards_count, bool is_cluster_mode);
  ~SubscriptionStorage();

  size_t GetShardCount() const { return shards_count_; }

  using ServerWeights = std::unordered_map<ServerId, size_t, ServerIdHasher>;

  using CommandCb = std::function<void(size_t shard, CommandPtr command)>;
  void SetSubscribeCallback(CommandCb);
  void SetUnsubscribeCallback(CommandCb);

  SubscriptionToken Subscribe(const std::string& channel,
                              Sentinel::UserMessageCallback cb,
                              CommandControl control);
  SubscriptionToken Psubscribe(const std::string& pattern,
                               Sentinel::UserPmessageCallback cb,
                               CommandControl control);

  void Unsubscribe(SubscriptionId subscription_id);

  void Stop();

  RawPubsubClusterStatistics GetStatistics() const;

  void SetCommandControl(const CommandControl& control);
  void SetRebalanceMinInterval(std::chrono::milliseconds interval);

  void RequestRebalance(size_t shard_idx, ServerWeights weights);

  void DoRebalance(size_t shard_idx, ServerWeights weights);

  void SwitchToNonClusterMode();

 private:
  struct ChannelName {
    ChannelName() = default;
    ChannelName(std::string channel, bool pattern)
        : channel(std::move(channel)), pattern(pattern) {}

    std::string channel;
    bool pattern{false};
  };

  using FsmPtr = std::shared_ptr<shard_subscriber::Fsm>;

  struct RebalanceState {
    RebalanceState(size_t shard_idx, ServerWeights weights);

    const size_t shard_idx;
    ServerWeights weights;
    size_t sum_weights{0};
    size_t total_connections{0};
    std::map<ServerId, std::vector<std::pair<ChannelName, FsmPtr>>>
        subscriptions_by_server;
    std::map<ServerId, size_t> need_subscription_count;
  };

  void RebalanceGatherSubscriptions(RebalanceState& state);
  void RebalanceCalculateNeedCount(RebalanceState& state);
  void RebalanceMoveSubscriptions(RebalanceState& state);

  size_t GetChannelsCountApprox() const;

  PubsubShardStatistics GetShardStatistics(size_t shard_idx) const;

  void ReadActions(FsmPtr fsm, const ChannelName& channel_name);

  void HandleChannelAction(FsmPtr fsm, shard_subscriber::Action action,
                           const ChannelName& channel_name);

  void HandleServerStateChanged(
      const std::shared_ptr<shard_subscriber::Fsm>& fsm,
      const ChannelName& channel_name, ServerId server_id,
      shard_subscriber::Event::Type event_type);

  SubscriptionId GetNextSubscriptionId();

  template <class Map>
  void DeleteChannel(Map& callback_map, const ChannelName& channel_name,
                     const FsmPtr& fsm);

  template <class Map>
  bool DoUnsubscribe(Map& callback_map, SubscriptionId id);

  CommandPtr PrepareUnsubscribeCommand(const ChannelName& channel_name);

  enum class SubscriberEvent { kSubscriberConnected, kSubscriberDisconnected };
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

  static void UnsubscribeCallbackNone(ServerId, const std::string& /*channel*/,
                                      size_t /*count*/) {}

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
  };
  using MessageCallback = std::function<void(
      ServerId, const std::string& channel, const std::string& message)>;
  using PmessageCallback = std::function<void(
      ServerId, const std::string& pattern, const std::string& channel,
      const std::string& message)>;

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
  };
  struct PChannelInfo {
    std::map<SubscriptionId, Sentinel::UserPmessageCallback> callbacks;
    CommandControl control;
    // shard -> Fsm
    std::vector<ShardChannelInfo> info;
    size_t active_fsm_count{0};
  };

  // (p)channel -> subscription_id -> callback
  using CallbackMap = std::map<std::string, ChannelInfo>;
  using PcallbackMap = std::map<std::string, PChannelInfo>;

  size_t shards_count_;
  bool is_cluster_mode_;
  CommandCb subscribe_callback_;
  CommandCb unsubscribe_callback_;

  mutable std::mutex mutex_;
  size_t shard_rotate_counter_;
  CallbackMap callback_map_;
  PcallbackMap pattern_callback_map_;
  SubscriptionId next_subscription_id_{1};
  CommandControl common_command_control_;

  std::vector<std::unique_ptr<SubscriptionRebalanceScheduler>>
      rebalance_schedulers_;
};

}  // namespace redis

USERVER_NAMESPACE_END
