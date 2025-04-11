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
        impl::SubscribeSentinel& subscribe_sentinel,
        std::string channel,
        const CommandControl& command_control
    );
    ~SubscriptionQueue();

    SubscriptionQueue(const SubscriptionQueue&) = delete;
    SubscriptionQueue& operator=(const SubscriptionQueue&) = delete;

    void SetMaxLength(size_t length);

    bool PopMessage(Item& msg_ptr);

    void Unsubscribe();

private:
    template <typename T = Item>
    std::enable_if_t<std::is_same<T, ChannelSubscriptionQueueItem>::value, impl::SubscriptionToken>
    GetSubscriptionToken(
        impl::SubscribeSentinel& subscribe_sentinel,
        std::string channel,
        const CommandControl& command_control
    );

    template <typename T = Item>
    std::enable_if_t<std::is_same<T, PatternSubscriptionQueueItem>::value, impl::SubscriptionToken>
    GetSubscriptionToken(
        impl::SubscribeSentinel& subscribe_sentinel,
        std::string pattern,
        const CommandControl& command_control
    );

    template <typename T = Item>
    std::enable_if_t<std::is_same<T, ShardedSubscriptionQueueItem>::value, impl::SubscriptionToken>
    GetSubscriptionToken(
        impl::SubscribeSentinel& subscribe_sentinel,
        std::string channel,
        const CommandControl& command_control
    );

    // Messages could come out-of-order due to Redis limitations. Non FIFO is fine
    using Queue = concurrent::NonFifoMpscQueue<Item>;
    using Outcome = impl::Sentinel::Outcome;

    std::shared_ptr<Queue> queue_;
    typename Queue::Producer producer_;
    typename Queue::Consumer consumer_;
    std::unique_ptr<impl::SubscriptionToken> token_;
};

extern template class SubscriptionQueue<ChannelSubscriptionQueueItem>;
extern template class SubscriptionQueue<PatternSubscriptionQueueItem>;
extern template class SubscriptionQueue<ShardedSubscriptionQueueItem>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
