#pragma once

#include <memory>
#include <string>

#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <userver/concurrent/queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

struct ChannelSubscriptionQueueItem {
    std::string message;

    ChannelSubscriptionQueueItem() = default;
    explicit ChannelSubscriptionQueueItem(std::string message) : message(std::move(message)) {}
};

struct PatternSubscriptionQueueItem {
    std::string channel;
    std::string message;

    PatternSubscriptionQueueItem() = default;
    PatternSubscriptionQueueItem(std::string channel, std::string message)
        : channel(std::move(channel)), message(std::move(message)) {}
};

struct ShardedSubscriptionQueueItem {
    std::string message;

    ShardedSubscriptionQueueItem() = default;
    explicit ShardedSubscriptionQueueItem(std::string message) : message(std::move(message)) {}
};

template <typename Item>
class SubscriptionQueue {
public:
    SubscriptionQueue(
        USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
        std::string channel,
        const USERVER_NAMESPACE::redis::CommandControl& command_control
    );
    ~SubscriptionQueue();

    SubscriptionQueue(const SubscriptionQueue&) = delete;
    SubscriptionQueue& operator=(const SubscriptionQueue&) = delete;

    void SetMaxLength(size_t length);

    bool PopMessage(Item& msg_ptr);

    void Unsubscribe();

private:
    template <typename T = Item>
    std::enable_if_t<std::is_same<T, ChannelSubscriptionQueueItem>::value, USERVER_NAMESPACE::redis::SubscriptionToken>
    GetSubscriptionToken(
        USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
        std::string channel,
        const USERVER_NAMESPACE::redis::CommandControl& command_control
    );

    template <typename T = Item>
    std::enable_if_t<std::is_same<T, PatternSubscriptionQueueItem>::value, USERVER_NAMESPACE::redis::SubscriptionToken>
    GetSubscriptionToken(
        USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
        std::string pattern,
        const USERVER_NAMESPACE::redis::CommandControl& command_control
    );

    template <typename T = Item>
    std::enable_if_t<std::is_same<T, ShardedSubscriptionQueueItem>::value, USERVER_NAMESPACE::redis::SubscriptionToken>
    GetSubscriptionToken(
        USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
        std::string channel,
        const USERVER_NAMESPACE::redis::CommandControl& command_control
    );

    // Messages could come out-of-order due to Redis limitations. Non FIFO is fine
    using Queue = concurrent::NonFifoMpscQueue<Item>;
    using Outcome = USERVER_NAMESPACE::redis::Sentinel::Outcome;

    std::shared_ptr<Queue> queue_;
    typename Queue::Producer producer_;
    typename Queue::Consumer consumer_;
    std::unique_ptr<USERVER_NAMESPACE::redis::SubscriptionToken> token_;
};

extern template class SubscriptionQueue<ChannelSubscriptionQueueItem>;
extern template class SubscriptionQueue<PatternSubscriptionQueueItem>;
extern template class SubscriptionQueue<ShardedSubscriptionQueueItem>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
