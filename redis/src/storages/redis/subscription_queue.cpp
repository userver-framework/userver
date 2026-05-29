#include "subscription_queue.hpp"

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

template <typename Item>
template <typename T>
SubscriptionQueue<Item>::SubscriptionQueue(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channels,
    const CommandControl& command_control
)
    : queue_(Queue::Create()),
      producer_(queue_->GetProducer()),
      consumer_(queue_->GetConsumer())
{
    token_.token = GetSubscriptionToken(subscribe_sentinel, std::move(channels), command_control);
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
template <typename T>
requires std::is_same_v<T, ChannelSubscriptionQueueItem>
SubscriptionQueue<Item>::TokenType SubscriptionQueue<Item>::GetSubscriptionToken(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channels,
    const CommandControl& command_control
) {
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
        ret.emplace_back(subscribe_sentinel.Subscribe(channel, callback, command_control));
    }
    return ret;
}

template <typename Item>
template <typename T>
requires std::is_same_v<T, PatternSubscriptionQueueItem>
SubscriptionQueue<Item>::TokenType SubscriptionQueue<Item>::GetSubscriptionToken(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> patterns,
    const CommandControl& command_control
) {
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

template <typename Item>
template <typename T>
requires std::is_same_v<T, ShardedSubscriptionQueueItem>
SubscriptionQueue<Item>::TokenType SubscriptionQueue<Item>::GetSubscriptionToken(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channels,
    const CommandControl& command_control
) {
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
        ret.emplace_back(subscribe_sentinel.Ssubscribe(channel, callback, command_control));
    }
    return ret;
}

template class SubscriptionQueue<ChannelSubscriptionQueueItem>;
template class SubscriptionQueue<PatternSubscriptionQueueItem>;
template class SubscriptionQueue<ShardedSubscriptionQueueItem>;

template SubscriptionQueue<ChannelSubscriptionQueueItem>::SubscriptionQueue(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channel,
    const CommandControl& command_control
);
template SubscriptionQueue<PatternSubscriptionQueueItem>::SubscriptionQueue(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channel,
    const CommandControl& command_control
);
template SubscriptionQueue<ShardedSubscriptionQueueItem>::SubscriptionQueue(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channel,
    const CommandControl& command_control
);

}  // namespace storages::redis

USERVER_NAMESPACE_END
