#include "subscription_storage.hpp"

#include <memory>
#include <stdexcept>
#include <type_traits>

#include <userver/logging/log.hpp>

#include <storages/redis/impl/command.hpp>
#include <storages/redis/impl/subscription_rebalance_scheduler.hpp>
#include <userver/storages/redis/impl/reply.hpp>
#include "cluster_subscription_storage.hpp"

USERVER_NAMESPACE_BEGIN

namespace redis {

SubscriptionToken::SubscriptionToken(
    std::weak_ptr<SubscriptionStorageBase> storage,
    SubscriptionId subscription_id)
    : storage_(storage), subscription_id_(subscription_id) {}

SubscriptionToken::SubscriptionToken(SubscriptionToken&& token) noexcept
    : storage_(std::move(token.storage_)),
      subscription_id_(token.subscription_id_) {
  token.subscription_id_ = 0;
}

SubscriptionToken& SubscriptionToken::operator=(
    SubscriptionToken&& token) noexcept {
  Unsubscribe();
  storage_ = token.storage_;
  subscription_id_ = token.subscription_id_;
  token.subscription_id_ = 0;
  return *this;
}

void SubscriptionToken::Unsubscribe() {
  if (subscription_id_ > 0) {
    LOG_DEBUG() << "Unsubscribe id=" << subscription_id_;
    auto storage = storage_.lock();
    if (storage) storage->Unsubscribe(subscription_id_);
    subscription_id_ = 0;
  }
}

SubscriptionToken::~SubscriptionToken() { Unsubscribe(); }

SubscriptionStorageBase::RebalanceState::RebalanceState(size_t shard_idx,
                                                        ServerWeights _weights)
    : shard_idx(shard_idx), weights(std::move(_weights)) {
  for (const auto& weight_item : weights) {
    sum_weights += weight_item.second;
    LOG_DEBUG() << "rebalance shard=" << shard_idx
                << " server_id=" << weight_item.first.GetId()
                << " weight=" << weight_item.second;
  }
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::DoRebalance(size_t shard_idx,
                                            ServerWeights weights) {
  RebalanceState state(shard_idx, std::move(weights));
  if (!state.sum_weights) return;

  const std::lock_guard<std::mutex> lock(mutex_);
  if (callback_map_.empty() && pattern_callback_map_.empty()) return;
  LOG_INFO() << "Start rebalance for shard " << shard_idx;

  RebalanceGatherSubscriptions(state, callback_map_, pattern_callback_map_);
  RebalanceCalculateNeedCount(state);
  RebalanceMoveSubscriptions(state);
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::RebalanceMoveSubscriptions(RebalanceState&
                                                               state) {
  auto shard_idx = state.shard_idx;
  auto& subscriptions_by_server = state.subscriptions_by_server;
  auto& needs = state.need_subscription_count;

  auto needs_iter = needs.begin();
  for (auto& subscriptions_item : subscriptions_by_server) {
    const auto& server_id = subscriptions_item.first;
    auto& subscriptions = subscriptions_item.second;
    const size_t need = needs[server_id];

    if (subscriptions.size() > need) {
      utils::Shuffle(subscriptions);
      while (subscriptions.size() > need) {
        auto channel_name = std::move(subscriptions.back().first);
        auto fsm = std::move(subscriptions.back().second);
        subscriptions.pop_back();
        while (subscriptions_by_server[needs_iter->first].size() >=
               needs_iter->second)
          ++needs_iter;

        const auto& new_server_id = needs_iter->first;

        LOG_INFO() << "move subscription on '" << channel_name.channel
                   << "' from server_id=" << fsm->GetCurrentServerId().GetId()
                   << " (" << fsm->GetCurrentServerId().GetDescription()
                   << ") to server_id=" << new_server_id.GetId() << " ("
                   << new_server_id.GetDescription()
                   << "), shard_idx=" << shard_idx;

        shard_subscriber::Event event;
        event.type = shard_subscriber::Event::Type::kRebalanceRequested;
        event.server_id = new_server_id;
        fsm->OnEvent(event);
        ReadActions(fsm, channel_name);

        subscriptions_by_server[new_server_id].emplace_back(
            std::move(channel_name), std::move(fsm));
      }
    }
  }
}

template <typename CallbackMap, typename PcallbackMap>
size_t SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::GetChannelsCountApprox() const {
  const std::lock_guard<std::mutex> lock(mutex_);
  return callback_map_.size() + pattern_callback_map_.size();
}

template <typename CallbackMap, typename PcallbackMap>
RawPubsubClusterStatistics SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::GetStatistics() const {
  RawPubsubClusterStatistics cluster_stats;
  for (size_t i = 0; i < shards_count_; i++) {
    cluster_stats.by_shard.push_back(GetShardStatistics(i));
  }
  return cluster_stats;
}

template <typename CallbackMap, typename PcallbackMap>
PubsubShardStatistics SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::GetShardStatistics(size_t shard_idx) const {
  PubsubShardStatistics shard_stats;
  shard_stats.shard_name = implemented_.GetShardName(shard_idx);
  shard_stats.by_channel.reserve(GetChannelsCountApprox());

  {
    const std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& channel_item : callback_map_) {
      const auto& channel_info = channel_item.second;
      const auto& info = channel_info.GetInfo(shard_idx);

      const auto& name = channel_item.first;
      if (info.fsm) shard_stats.by_channel.emplace(name, info.GetStatistics());
    }
    for (const auto& pattern_item : pattern_callback_map_) {
      const auto& pattern_info = pattern_item.second;
      const auto& info = pattern_info.GetInfo(shard_idx);

      const auto& name = pattern_item.first;
      if (info.fsm) shard_stats.by_channel.emplace(name, info.GetStatistics());
    }
  }
  return shard_stats;
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::Unsubscribe(SubscriptionId subscription_id) {
  if (DoUnsubscribe(callback_map_, subscription_id)) return;
  if (DoUnsubscribe(pattern_callback_map_, subscription_id)) return;

  LOG_ERROR() << "Unsubscribe called with invalid subscription_id: "
              << subscription_id;
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::ReadActions(FsmPtr fsm,
                                            const ChannelName& channel_name) {
  UASSERT(fsm);
  const auto actions = fsm->PopAllPendingActions();
  for (const auto& action : actions) {
    HandleChannelAction(fsm, action, channel_name);
  }
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::
    SubscriptionStorageImpl<CallbackMap, PcallbackMap>::HandleChannelAction(
        FsmPtr fsm, shard_subscriber::Action action,
        const ChannelName& channel_name) {
  UASSERT(fsm);
  CommandPtr cmd;
  std::weak_ptr<shard_subscriber::Fsm> weak_fsm = fsm;
  const size_t shard = fsm->GetShard();
  auto self = implemented_.shared_from_this();

  switch (action.type) {
    case shard_subscriber::Action::Type::kSubscribe: {
      /*
       * Use weak ptr as Fsm may be destroyed by unsubscribe() earlier
       * than SUBSCRIBE reply is received.
       */
      auto subscribe_cb = [this, self, weak_fsm, channel_name](
                              ServerId server_id, SubscriberEvent event) {
        auto fsm = weak_fsm.lock();
        if (!fsm) {
          // possible after Stop() only
          return;
        }

        HandleServerStateChanged(fsm, channel_name, server_id,
                                 EventTypeFromSubscriberEvent(event));
      };
      cmd =
          PrepareSubscribeCommand(channel_name, std::move(subscribe_cb), shard);
      cmd->control.force_server_id = action.server_id;
      subscribe_callback_(fsm->GetShard(), cmd);
      break;
    }

    case shard_subscriber::Action::Type::kUnsubscribe:
      cmd = PrepareUnsubscribeCommand(channel_name);
      cmd->control.force_server_id = action.server_id;
      unsubscribe_callback_(fsm->GetShard(), cmd);
      break;

    case shard_subscriber::Action::Type::kDeleteFsm:
      if (channel_name.pattern)
        DeleteChannel(pattern_callback_map_, channel_name, fsm);
      else
        DeleteChannel(callback_map_, channel_name, fsm);
      break;
  }
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<CallbackMap,
                                                      PcallbackMap>::
    HandleServerStateChanged(const std::shared_ptr<shard_subscriber::Fsm>& fsm,
                             const ChannelName& channel_name,
                             ServerId server_id,
                             shard_subscriber::Event::Type event_type) {
  UASSERT(fsm);
  shard_subscriber::Event event;
  event.type = event_type;
  event.server_id = server_id;

  std::lock_guard<std::mutex> lock(mutex_);
  fsm->OnEvent(event);
  ReadActions(fsm, channel_name);
}

template <typename CallbackMap, typename PcallbackMap>
template <class Map>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::DeleteChannel(Map& callback_map,
                                              const ChannelName& channel_name,
                                              const FsmPtr& fsm) {
  UASSERT(fsm);
  const auto& channel = channel_name.channel;
  auto it = callback_map.find(channel);
  if (it == callback_map.end()) {
    LOG_ERROR() << "channel=" << channel << " not found in callback_map";
    return;
  }
  if (!it->second.callbacks.empty()) {
    LOG_ERROR() << "got DeleteChannel request but callbacks map is not empty "
                   "for channel="
                << channel;

    shard_subscriber::Event event;
    event.type = shard_subscriber::Event::Type::kSubscribeRequested;

    fsm->OnEvent(event);
    ReadActions(fsm, channel_name);
    return;
  }
  if (!--it->second.active_fsm_count) {
    callback_map.erase(it);
  }
}

template <typename CallbackMap, typename PcallbackMap>
template <class Map>
bool SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::DoUnsubscribe(Map& callback_map,
                                              SubscriptionId subscription_id) {
  std::unique_lock<std::mutex> lock(mutex_);
  for (auto& it1 : callback_map) {
    const auto& key = it1.first;
    auto& m = it1.second;

    auto it2 = m.callbacks.find(subscription_id);
    if (it2 != m.callbacks.end()) {
      m.callbacks.erase(it2);
      if (m.callbacks.empty()) {
        if (unsubscribe_callback_) {
          shard_subscriber::Event event;
          event.type = shard_subscriber::Event::Type::kUnsubscribeRequested;

          ChannelName channel_name;
          channel_name.channel = key;
          channel_name.pattern = std::is_same_v<Map, PcallbackMap>;

          for (size_t i = 0; i < shards_count_; ++i) {
            auto& fsm = m.GetInfo(i).fsm;
            if (!fsm) continue;
            fsm->OnEvent(event);

            ReadActions(fsm, channel_name);
          }
        }
      }
      return true;
    }
  }

  return false;
}

template <typename CallbackMap, typename PcallbackMap>
CommandPtr SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::PrepareUnsubscribeCommand(const ChannelName&
                                                              channel_name) {
  const auto* command_name =
      channel_name.pattern ? "PUNSUBSCRIBE" : "UNSUBSCRIBE";
  const auto& channel = channel_name.channel;
  return PrepareCommand(CmdArgs{command_name, channel}, ReplyCallback{},
                        common_command_control_);
}

template <typename CallbackMap, typename PcallbackMap>
shard_subscriber::Event::Type SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::EventTypeFromSubscriberEvent(SubscriberEvent
                                                                 event) {
  switch (event) {
    case SubscriberEvent::kSubscriberConnected:
      return shard_subscriber::Event::Type::kSubscribeReplyOk;
    case SubscriberEvent::kSubscriberDisconnected:
      return shard_subscriber::Event::Type::kSubscribeReplyError;
  }
  auto message =
      "unknown SubscriberEvent: " + std::to_string(static_cast<int>(event));
  throw std::runtime_error(message);
}

template <typename CallbackMap, typename PcallbackMap>
CommandPtr SubscriptionStorageBase::
    SubscriptionStorageImpl<CallbackMap, PcallbackMap>::PrepareSubscribeCommand(
        const ChannelName& channel_name, SubscribeCb cb, size_t shard_idx) {
  const auto message_callback = [this, shard_idx](ServerId server_id,
                                                  const std::string& channel,
                                                  const std::string& message) {
    OnMessage(server_id, channel, message, shard_idx);
  };
  const auto pmessage_callback = [this, shard_idx](ServerId server_id,
                                                   const std::string& pattern,
                                                   const std::string& channel,
                                                   const std::string& message) {
    OnPmessage(server_id, pattern, channel, message, shard_idx);
  };
  const auto subscribe_callback = [cb](ServerId server_id,
                                       const std::string& /*channel*/,
                                       size_t response) {
    cb(server_id, response > 0 ? SubscriberEvent::kSubscriberConnected
                               : SubscriberEvent::kSubscriberDisconnected);
  };
  const auto unsubscribe_callback = [cb](ServerId server_id,
                                         const std::string& /*channel*/,
                                         size_t /*response*/) {
    cb(server_id, SubscriberEvent::kSubscriberDisconnected);
  };

  const auto& channel = channel_name.channel;
  const auto* cmd_name = channel_name.pattern ? "PSUBSCRIBE" : "SUBSCRIBE";
  return PrepareCommand(
      CmdArgs{cmd_name, channel},
      [channel_name, pmessage_callback, message_callback, subscribe_callback,
       unsubscribe_callback](const CommandPtr&, ReplyPtr reply) {
        if (!reply->IsOk() || !reply->data || !reply->data.IsArray()) {
          // Subscribe error or disconnect
          subscribe_callback(reply->server_id, channel_name.channel, 0);
          return;
        }

        if (channel_name.pattern) {
          Sentinel::OnPsubscribeReply(pmessage_callback, subscribe_callback,
                                      unsubscribe_callback, reply);
        } else {
          Sentinel::OnSubscribeReply(message_callback, subscribe_callback,
                                     unsubscribe_callback, reply);
        }
      },
      common_command_control_);
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::OnMessage(ServerId server_id,
                                          const std::string& channel,
                                          const std::string& message,
                                          size_t shard_idx) {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& m = callback_map_.at(channel);
    for (const auto& it : m.callbacks) {
      try {
        it.second(channel, message);
      } catch (const std::exception& e) {
        LOG_ERROR() << "Unhandled exception in subscriber: " << e.what();
      }
    }

    m.GetInfo(shard_idx).AccountMessage(server_id, message.size());
  } catch (const std::out_of_range& e) {
    LOG_ERROR() << "Got MESSAGE while not subscribed on it, channel="
                << channel;
  }
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::OnPmessage(ServerId server_id,
                                           const std::string& pattern,
                                           const std::string& channel,
                                           const std::string& message,
                                           size_t shard_idx) {
  try {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& m = pattern_callback_map_.at(pattern);
    for (const auto& it : m.callbacks) {
      try {
        it.second(pattern, channel, message);
      } catch (const std::exception& e) {
        LOG_ERROR() << "Unhandled exception in subscriber: " << e.what();
      }
    }

    m.GetInfo(shard_idx).AccountMessage(server_id, message.size());
  } catch (const std::out_of_range& e) {
    LOG_ERROR() << "Got PMESSAGE while not subscribed on it, channel="
                << channel;
  }
}

void SubscriptionStorageBase::ShardChannelInfo::AccountMessage(
    ServerId server_id, size_t message_size) {
  UASSERT(fsm);
  auto current_server_id = fsm->GetCurrentServerId();
  if (current_server_id == server_id)
    statistics.AccountMessage(message_size);
  else {
    // TODO: better handling in https://st.yandex-team.ru/TAXICOMMON-604
    LOG_LIMITED_ERROR()
        << "Alien message got on SUBSCRIBE, fsm=" << fsm.get()
        << ", origin server_id = " << server_id.GetId()
        << ", server = " << server_id.GetDescription()
        << ", current server_id = " << current_server_id.GetId()
        << ", current server = " << current_server_id.GetDescription()
        << ". Possible while rebalancing. If these messages are "
           "regular, it is a bug in PUBSUB "
           "client implementation.";

    statistics.AccountAlienMessage();
  }
}

// logically better as non-static func
// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::RebalanceCalculateNeedCount(RebalanceState&
                                                                state) {
  auto& weights = state.weights;
  auto& needs = state.need_subscription_count;
  const auto total_connections = state.total_connections;
  const auto sum_weights = state.sum_weights;

  size_t rem = total_connections;
  size_t rem_sum_weights = 0;
  for (auto& [server_id, weight] : weights) {
    /// How many connections can be assigned to this server
    const size_t need = total_connections * weight / sum_weights;
    needs[server_id] = need;
    /// Modifying weight??
    weight = total_connections * weight % sum_weights;
    rem_sum_weights += weight;
    if (rem < need) throw std::logic_error("something went wrong (rem < need)");
    rem -= need;
  }
  if (rem > weights.size())
    throw std::logic_error("incorrect rem count (rem > size)");

  for (; rem; --rem) {
    if (!rem_sum_weights) throw std::logic_error("incorrect rem_sum_weights");
    size_t current = utils::RandRange(rem_sum_weights);
    for (auto& server_id_weight : weights) {
      const auto& server_id = server_id_weight.first;
      auto& weight = server_id_weight.second;
      if (current < weight) {
        ++needs[server_id];
        rem_sum_weights -= weight;
        weight = 0;
        break;
      }
      current -= weight;
    }
  }
}

/// make subscriptions_by_server  and total_connections by callback_map_
/// How many channel_names/patterns is listened on each connection
template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap,
    PcallbackMap>::RebalanceGatherSubscriptions(RebalanceState& state,
                                                CallbackMap& callback_map,
                                                PcallbackMap& pcallback_map) {
  auto shard_idx = state.shard_idx;
  auto& subscriptions_by_server = state.subscriptions_by_server;
  auto& total_connections = state.total_connections;

  for (const auto& channel_item : callback_map) {
    const auto& channel_info = channel_item.second;
    const auto& fsm = channel_info.GetInfo(shard_idx).fsm;
    if (!fsm || !fsm->CanBeRebalanced()) {
      continue;
    }
    ++total_connections;
    subscriptions_by_server[fsm->GetCurrentServerId()].emplace_back(
        ChannelName(channel_item.first, false), fsm);
  }
  for (const auto& pattern_item : pcallback_map) {
    const auto& pattern_info = pattern_item.second;
    const auto& fsm = pattern_info.GetInfo(shard_idx).fsm;
    if (!fsm || !fsm->CanBeRebalanced()) {
      continue;
    }
    ++total_connections;
    subscriptions_by_server[fsm->GetCurrentServerId()].emplace_back(
        ChannelName(pattern_item.first, true), fsm);
  }
}

template <typename CallbackMap, typename PcallbackMap>
void SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::SetCommandControl(const CommandControl&
                                                      control) {
  std::lock_guard<std::mutex> lock(mutex_);
  common_command_control_ = control;
  common_command_control_.max_retries = 1;
}

template <typename CallbackMap, typename PcallbackMap>
SubscriptionToken SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::Subscribe(const std::string& channel,
                                          Sentinel::UserMessageCallback cb,
                                          CommandControl control) {
  size_t id = 0;
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    id = GetNextSubscriptionId();
  }
  SubscriptionToken token(implemented_.shared_from_this(), id);
  LOG_DEBUG() << "Subscribe on channel=" << channel << " id=" << id;
  implemented_.SubscribeImpl(channel, std::move(cb), std::move(control), id);
  return token;
}

template <typename CallbackMap, typename PcallbackMap>
SubscriptionToken SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::Psubscribe(const std::string& channel,
                                           Sentinel::UserPmessageCallback cb,
                                           CommandControl control) {
  size_t id = 0;
  {
    const std::lock_guard<std::mutex> lock(mutex_);
    id = GetNextSubscriptionId();
  }
  SubscriptionToken token(implemented_.shared_from_this(), id);
  LOG_DEBUG() << "Psubscribe on channel=" << channel << " id=" << id;
  implemented_.PsubscribeImpl(channel, std::move(cb), std::move(control), id);
  return token;
}

template <typename CallbackMap, typename PcallbackMap>
SubscriptionId SubscriptionStorageBase::SubscriptionStorageImpl<
    CallbackMap, PcallbackMap>::GetNextSubscriptionId() {
  return next_subscription_id_++;
}

/// Explicit instantiation
template class SubscriptionStorageBase::SubscriptionStorageImpl<
    ClusterSubscriptionStorage::CallbackMap,
    ClusterSubscriptionStorage::PcallbackMap>;

SubscriptionStorage::SubscriptionStorage(
    const std::shared_ptr<ThreadPools>& thread_pools, size_t shards_count,
    bool is_cluster_mode,
    std::shared_ptr<const std::vector<std::string>> shard_names)
    : storage_impl_(shards_count, *this),
      shard_names_(std::move(shard_names)),
      is_cluster_mode_(is_cluster_mode),
      shard_rotate_counter_(utils::RandRange(storage_impl_.shards_count_)) {
  for (size_t shard_idx = 0; shard_idx < storage_impl_.shards_count_;
       shard_idx++) {
    rebalance_schedulers_.emplace_back(
        std::make_unique<SubscriptionRebalanceScheduler>(
            thread_pools->GetSentinelThreadPool(), *this, shard_idx));
  }
}

SubscriptionStorage::~SubscriptionStorage() = default;

void SubscriptionStorage::SetSubscribeCallback(CommandCb cb) {
  storage_impl_.subscribe_callback_ = std::move(cb);
}

void SubscriptionStorage::SetUnsubscribeCallback(CommandCb cb) {
  storage_impl_.unsubscribe_callback_ = std::move(cb);
}

SubscriptionToken SubscriptionStorage::Subscribe(
    const std::string& channel, Sentinel::UserMessageCallback cb,
    CommandControl control) {
  return storage_impl_.Subscribe(channel, std::move(cb), std::move(control));
}

SubscriptionToken SubscriptionStorage::Psubscribe(
    const std::string& pattern, Sentinel::UserPmessageCallback cb,
    CommandControl control) {
  return storage_impl_.Psubscribe(pattern, std::move(cb), std::move(control));
}

void SubscriptionStorage::Unsubscribe(SubscriptionId subscription_id) {
  storage_impl_.Unsubscribe(subscription_id);
}

void SubscriptionStorage::Stop() {
  {
    std::unique_lock<std::mutex> lock(storage_impl_.mutex_);
    storage_impl_.callback_map_.clear();
    storage_impl_.pattern_callback_map_.clear();
  }
  rebalance_schedulers_.clear();
}

void SubscriptionStorage::SetCommandControl(const CommandControl& control) {
  storage_impl_.SetCommandControl(control);
}

void SubscriptionStorage::SetRebalanceMinInterval(
    std::chrono::milliseconds interval) {
  for (auto& scheduler : rebalance_schedulers_)
    scheduler->SetRebalanceMinInterval(interval);
}

void SubscriptionStorage::RequestRebalance(size_t shard_idx,
                                           ServerWeights weights) {
  rebalance_schedulers_[shard_idx]->RequestRebalance(std::move(weights));
}

void SubscriptionStorage::DoRebalance(size_t shard_idx, ServerWeights weights) {
  /// Rebalances subscriptions between instances of shard
  if (shard_idx >= storage_impl_.shards_count_)
    throw std::runtime_error("requested rebalance for non-existing shard (" +
                             std::to_string(shard_idx) + " >= " +
                             std::to_string(storage_impl_.shards_count_) + ')');

  storage_impl_.DoRebalance(shard_idx, std::move(weights));
}

void SubscriptionStorage::SwitchToNonClusterMode() {
  std::lock_guard<std::mutex> lock(storage_impl_.mutex_);
  UASSERT(is_cluster_mode_);
  is_cluster_mode_ = false;
  LOG_INFO() << "SwitchToNonClusterMode for subscription storage";

  for (auto& channel_item : storage_impl_.callback_map_) {
    auto& channel_info = channel_item.second;
    if (channel_info.callbacks.empty()) continue;

    auto& infos = channel_info.info;
    ChannelName channel_name(channel_item.first, false);
    for (size_t i = 0; i < storage_impl_.shards_count_; ++i) {
      if (!infos[i].fsm) {
        ++channel_info.active_fsm_count;
        infos[i].fsm = std::make_shared<shard_subscriber::Fsm>(i);
        LOG_DEBUG() << "Create fsm for non-cluster: shard_idx=" << i
                    << ", channel_name=" << channel_name.channel;
        storage_impl_.ReadActions(infos[i].fsm, channel_name);
      }
    }
  }

  for (auto& channel_item : storage_impl_.pattern_callback_map_) {
    auto& channel_info = channel_item.second;
    if (channel_info.callbacks.empty()) continue;

    auto& infos = channel_info.info;
    ChannelName channel_name(channel_item.first, true);
    for (size_t i = 0; i < storage_impl_.shards_count_; ++i) {
      if (!infos[i].fsm) {
        ++channel_info.active_fsm_count;
        infos[i].fsm = std::make_shared<shard_subscriber::Fsm>(i);
        LOG_DEBUG() << "Create fsm for non-cluster: shard_idx=" << i
                    << ", pattern_name=" << channel_name.channel;
        storage_impl_.ReadActions(infos[i].fsm, channel_name);
      }
    }
  }
}

RawPubsubClusterStatistics SubscriptionStorage::GetStatistics() const {
  return storage_impl_.GetStatistics();
}

void SubscriptionStorage::SubscribeImpl(const std::string& channel,
                                        Sentinel::UserMessageCallback cb,
                                        CommandControl control,
                                        SubscriptionId id) {
  /// In non cluster mode we are using our pubsub in specific environment
  /// where we actually use multiple non-clustered redises to prevent full
  /// connected network and save network bandwidth consumption (we do it so
  /// because ssubscribe/spublish was not available before redis 7.0). So we
  /// have to subscribe to every shard to be able to receive published message.
  /// In cluster mode subscribe to only one shard because we do not use
  /// previously mentioned workaround. So each instance in cluster is connected
  const std::lock_guard<std::mutex> lock(storage_impl_.mutex_);
  auto insert_res = storage_impl_.callback_map_.emplace(channel, ChannelInfo());
  auto& map_iter = *insert_res.first;
  auto& channel_info = map_iter.second;
  auto& infos = channel_info.info;
  /// 1 fsm for cluster and shards_count fsms for non cluster
  channel_info.active_fsm_count =
      is_cluster_mode_ ? 1 : storage_impl_.shards_count_;

  ChannelName channel_name;
  channel_name.channel = channel;
  channel_name.pattern = false;

  if (insert_res.second) {
    // new channel
    channel_info.control = control;

    const size_t selected_shard_idx =
        is_cluster_mode_ ? shard_rotate_counter_++ % storage_impl_.shards_count_
                         : 0;
    infos.reserve(storage_impl_.shards_count_);
    for (size_t i = 0; i < storage_impl_.shards_count_; ++i) {
      const bool fake = is_cluster_mode_ && i != selected_shard_idx;
      infos.emplace_back(i, fake);
      if (!fake) storage_impl_.ReadActions(infos.back().fsm, channel_name);
    }
  } else {
    for (auto& info : infos) {
      if (!info.fsm) continue;
      shard_subscriber::Event event;
      event.type = shard_subscriber::Event::Type::kSubscribeRequested;
      info.fsm->OnEvent(event);
      storage_impl_.ReadActions(info.fsm, channel_name);
    }
  }

  storage_impl_.callback_map_[channel].callbacks[id] = std::move(cb);
}

void SubscriptionStorage::PsubscribeImpl(const std::string& pattern,
                                         Sentinel::UserPmessageCallback cb,
                                         CommandControl control,
                                         SubscriptionId id) {
  const std::lock_guard<std::mutex> lock(storage_impl_.mutex_);
  auto insert_res =
      storage_impl_.pattern_callback_map_.emplace(pattern, PChannelInfo());
  auto& map_iter = *insert_res.first;
  auto& channel_info = map_iter.second;
  auto& infos = channel_info.info;
  /// 1 fsm for cluster and shards_count fsms for non cluster
  ///  !!! Is this missing line a bug????
  channel_info.active_fsm_count =
      is_cluster_mode_ ? 1 : storage_impl_.shards_count_;

  ChannelName channel_name;
  channel_name.channel = pattern;
  channel_name.pattern = true;

  if (insert_res.second) {
    // new channel
    channel_info.control = control;

    const size_t selected_shard_idx =
        is_cluster_mode_ ? shard_rotate_counter_++ % storage_impl_.shards_count_
                         : 0;
    infos.reserve(storage_impl_.shards_count_);
    for (size_t i = 0; i < storage_impl_.shards_count_; ++i) {
      bool fake = is_cluster_mode_ && i != selected_shard_idx;
      infos.emplace_back(i, fake);
      if (!fake) storage_impl_.ReadActions(infos.back().fsm, channel_name);
    }
  } else {
    for (auto& info : infos) {
      if (!info.fsm) continue;
      shard_subscriber::Event event;
      event.type = shard_subscriber::Event::Type::kSubscribeRequested;
      info.fsm->OnEvent(event);
      storage_impl_.ReadActions(info.fsm, channel_name);
    }
  }

  storage_impl_.pattern_callback_map_[pattern].callbacks[id] = std::move(cb);
}

const std::string& SubscriptionStorage::GetShardName(size_t shard_idx) const {
  UASSERT(shard_names_);
  return shard_names_->at(shard_idx);
}

}  // namespace redis

USERVER_NAMESPACE_END
