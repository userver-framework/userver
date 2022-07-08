#pragma once

#include <memory>
#include <string>

#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <userver/engine/mpsc_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

struct ChannelSubscriptionQueueItem {
  std::string message;

  ChannelSubscriptionQueueItem() = default;
  explicit ChannelSubscriptionQueueItem(std::string message)
      : message(std::move(message)) {}
};

struct PatternSubscriptionQueueItem {
  std::string channel;
  std::string message;

  PatternSubscriptionQueueItem() = default;
  PatternSubscriptionQueueItem(std::string channel, std::string message)
      : channel(std::move(channel)), message(std::move(message)) {}
};

template <typename Item>
class SubscriptionQueue {
 public:
  SubscriptionQueue(
      USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
      std::string channel,
      const USERVER_NAMESPACE::redis::CommandControl& command_control);
  ~SubscriptionQueue();

  SubscriptionQueue(const SubscriptionQueue&) = delete;
  SubscriptionQueue& operator=(const SubscriptionQueue&) = delete;

  void SetMaxLength(size_t length);

  bool PopMessage(std::unique_ptr<Item>& msg_ptr);

  void Unsubscribe();

 private:
  template <typename T = Item>
  std::enable_if_t<std::is_same<T, ChannelSubscriptionQueueItem>::value,
                   USERVER_NAMESPACE::redis::SubscriptionToken>
  GetSubscriptionToken(
      USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
      std::string channel,
      const USERVER_NAMESPACE::redis::CommandControl& command_control);

  template <typename T = Item>
  std::enable_if_t<std::is_same<T, PatternSubscriptionQueueItem>::value,
                   USERVER_NAMESPACE::redis::SubscriptionToken>
  GetSubscriptionToken(
      USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
      std::string pattern,
      const USERVER_NAMESPACE::redis::CommandControl& command_control);

  using Queue = engine::MpscQueue<std::unique_ptr<Item>>;

  std::shared_ptr<Queue> queue_;
  typename Queue::Producer producer_;
  typename Queue::Consumer consumer_;
  std::unique_ptr<USERVER_NAMESPACE::redis::SubscriptionToken> token_;
};

extern template class SubscriptionQueue<ChannelSubscriptionQueueItem>;
extern template class SubscriptionQueue<PatternSubscriptionQueueItem>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
