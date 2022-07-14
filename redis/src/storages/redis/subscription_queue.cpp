#include "subscription_queue.hpp"

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <typename Item>
SubscriptionQueue<Item>::SubscriptionQueue(
    USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
    std::string channel,
    const USERVER_NAMESPACE::redis::CommandControl& command_control)
    : queue_(Queue::Create()),
      producer_(queue_->GetProducer()),
      consumer_(queue_->GetConsumer()),
      token_(std::make_unique<USERVER_NAMESPACE::redis::SubscriptionToken>(
          GetSubscriptionToken(subscribe_sentinel, std::move(channel),
                               command_control))) {}

template <typename Item>
SubscriptionQueue<Item>::~SubscriptionQueue() {
  Unsubscribe();
}

template <typename Item>
void SubscriptionQueue<Item>::SetMaxLength(size_t length) {
  queue_->SetSoftMaxSize(length);
}

template <typename Item>
bool SubscriptionQueue<Item>::PopMessage(std::unique_ptr<Item>& msg_ptr) {
  return consumer_.Pop(msg_ptr);
}

template <typename Item>
void SubscriptionQueue<Item>::Unsubscribe() {
  token_->Unsubscribe();
}

template <typename Item>
template <typename T>
std::enable_if_t<std::is_same<T, ChannelSubscriptionQueueItem>::value,
                 USERVER_NAMESPACE::redis::SubscriptionToken>
SubscriptionQueue<Item>::GetSubscriptionToken(
    USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
    std::string channel,
    const USERVER_NAMESPACE::redis::CommandControl& command_control) {
  return subscribe_sentinel.Subscribe(
      channel,
      [this](const std::string& channel, const std::string& message) {
        if (!producer_.PushNoblock(std::make_unique<Item>(message))) {
          // Use SubscriptionQueue::SetMaxLength() or
          // SubscriptionToken::SetMaxQueueLength() if limit is too low
          LOG_ERROR()
              << "failed to push message '" << message << "' from channel '"
              << channel
              << "' into subscription queue due to overflow (max length="
              << queue_->GetSoftMaxSize() << ')';
        }
      },
      command_control);
}

template <typename Item>
template <typename T>
std::enable_if_t<std::is_same<T, PatternSubscriptionQueueItem>::value,
                 USERVER_NAMESPACE::redis::SubscriptionToken>
SubscriptionQueue<Item>::GetSubscriptionToken(
    USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
    std::string pattern,
    const USERVER_NAMESPACE::redis::CommandControl& command_control) {
  return subscribe_sentinel.Psubscribe(
      pattern,
      [this](const std::string& pattern, const std::string& channel,
             const std::string& message) {
        if (!producer_.PushNoblock(std::make_unique<Item>(channel, message))) {
          // Use SubscriptionQueue::SetMaxLength() or
          // SubscriptionToken::SetMaxQueueLength() if limit is too low
          LOG_ERROR()
              << "failed to push pmessage '" << message << "' from channel '"
              << channel << "' from pattern '" << pattern
              << "' into subscription queue due to overflow (max length="
              << queue_->GetSoftMaxSize() << ')';
        }
      },
      command_control);
}

template class SubscriptionQueue<ChannelSubscriptionQueueItem>;
template class SubscriptionQueue<PatternSubscriptionQueueItem>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
