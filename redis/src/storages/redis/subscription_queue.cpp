#include "subscription_queue.hpp"

#include <utility>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <typename Item>
SubscriptionQueue<Item>::SubscriptionQueue()
    : queue_(Queue::Create()),
      producer_(queue_->GetProducer()),
      consumer_(queue_->GetConsumer())
{}

template <typename Item>
SubscriptionQueue<Item>::SubscriptionQueue(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> patterns,
    const CommandControl& command_control
)
requires std::is_same_v<Item, PatternSubscriptionQueueItem>
    : SubscriptionQueue()
{
    token_.token = GetSubscriptionToken(subscribe_sentinel, std::move(patterns), command_control);
}

template <typename Item>
SubscriptionQueue<Item>::SubscriptionQueue(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channels,
    const CommandControl& command_control,
    ChannelSubscriptionMode mode
)
requires std::is_same_v<Item, ChannelSubscriptionQueueItem>
    : SubscriptionQueue()
{
    token_.token = GetSubscriptionToken(subscribe_sentinel, std::move(channels), command_control, mode);
}

template <typename Item>
SubscriptionQueue<Item>::~SubscriptionQueue() {
    Unsubscribe();
}

template <typename Item>
void SubscriptionQueue<Item>::SetMaxLength(size_t length) {
    queue_->SetSoftMaxSize(length);
}

template <typename Item>
bool SubscriptionQueue<Item>::PopMessage(Item& msg_ptr) {
    return consumer_.Pop(msg_ptr);
}

template <typename Item>
void SubscriptionQueue<Item>::Unsubscribe() {
    token_.Unsubscribe();
}

template <typename Item>
SubscriptionQueue<Item>::TokenType SubscriptionQueue<Item>::GetSubscriptionToken(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channels,
    const CommandControl& command_control,
    ChannelSubscriptionMode mode
)
requires std::is_same_v<Item, ChannelSubscriptionQueueItem>
{
    std::vector<impl::SubscriptionToken> ret;
    ret.reserve(channels.size());
    auto callback = [this](const std::string& channel, const std::string& message) {
        Outcome result{Outcome::kOk};
        if (!producer_.PushNoblock(Item(channel, message))) {
            // Use SubscriptionQueue::SetMaxLength() or
            // SubscriptionToken::SetMaxQueueLength() if limit is too low
            LOG_ERROR()
                << "failed to push message '" << message << "' from channel '" << channel
                << "' into subscription queue due to overflow (max length=" << queue_->GetSoftMaxSize() << ')';
            // either this line
            result = Outcome::kOverflowDiscarded;
        }

        return result;
    };

    for (auto&& channel : channels) {
        switch (mode) {
            case ChannelSubscriptionMode::kSubscribe:
                ret.emplace_back(subscribe_sentinel.Subscribe(channel, callback, command_control));
                break;
            case ChannelSubscriptionMode::kSsubscribe:
                ret.emplace_back(subscribe_sentinel.Ssubscribe(channel, callback, command_control));
                break;
        }
    }
    return ret;
}

template <typename Item>
SubscriptionQueue<Item>::TokenType SubscriptionQueue<Item>::GetSubscriptionToken(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> patterns,
    const CommandControl& command_control
)
requires std::is_same_v<Item, PatternSubscriptionQueueItem>
{
    std::vector<impl::SubscriptionToken> ret;
    ret.reserve(patterns.size());
    auto callback = [this](const std::string& pattern, const std::string& channel, const std::string& message) {
        Outcome result{Outcome::kOk};
        if (!producer_.PushNoblock(Item(pattern, channel, message))) {
            // Use SubscriptionQueue::SetMaxLength() or
            // SubscriptionToken::SetMaxQueueLength() if limit is too low
            LOG_ERROR()
                << "failed to push pmessage '" << message << "' from channel '" << channel << "' from pattern '"
                << pattern << "' into subscription queue due to overflow (max length=" << queue_->GetSoftMaxSize()
                << ')';
            // either this line
            result = Outcome::kOverflowDiscarded;
        }

        return result;
    };

    for (auto&& pattern : patterns) {
        ret.emplace_back(subscribe_sentinel.Psubscribe(pattern, callback, command_control));
    }
    return ret;
}

template class SubscriptionQueue<ChannelSubscriptionQueueItem>;
template class SubscriptionQueue<PatternSubscriptionQueueItem>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
