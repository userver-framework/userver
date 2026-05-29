#pragma once

#include <memory>
#include <string>

#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <userver/concurrent/queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

struct ChannelSubscriptionQueueItem {
    std::string channel;
    std::string message;

    ChannelSubscriptionQueueItem() = default;
    explicit ChannelSubscriptionQueueItem(std::string channel, std::string message)
        : channel(std::move(channel)),
          message(std::move(message))
    {}
};

struct PatternSubscriptionQueueItem {
    std::string pattern;
    std::string channel;
    std::string message;

    PatternSubscriptionQueueItem() = default;
    PatternSubscriptionQueueItem(std::string pattern, std::string channel, std::string message)
        : pattern(std::move(pattern)),
          channel(std::move(channel)),
          message(std::move(message))
    {}
};

struct ShardedSubscriptionQueueItem {
    std::string channel;
    std::string message;

    ShardedSubscriptionQueueItem() = default;
    explicit ShardedSubscriptionQueueItem(std::string channel, std::string message)
        : channel(std::move(channel)),
          message(std::move(message))
    {}
};

template <typename Item>
class SubscriptionQueue {
public:
    template <typename T = Item>
    SubscriptionQueue(
        impl::SubscribeSentinel& subscribe_sentinel,
        std::vector<std::string> channel,
        const CommandControl& command_control
    );

    ~SubscriptionQueue();

    SubscriptionQueue(const SubscriptionQueue&) = delete;
    SubscriptionQueue& operator=(const SubscriptionQueue&) = delete;

    void SetMaxLength(size_t length);

    bool PopMessage(Item& msg_ptr);

    void Unsubscribe();

private:
    using TokenType = std::vector<impl::SubscriptionToken>;
    struct MultiToken {
        TokenType token;
        void Unsubscribe() {
            for (auto& t : token) {
                t.Unsubscribe();
            }
        }
    };

    template <typename T = Item>
    requires std::is_same_v<T, ChannelSubscriptionQueueItem>
    TokenType GetSubscriptionToken(
        impl::SubscribeSentinel& subscribe_sentinel,
        std::vector<std::string> channels,
        const CommandControl& command_control
    );

    template <typename T = Item>
    requires std::is_same_v<T, PatternSubscriptionQueueItem>
    TokenType GetSubscriptionToken(
        impl::SubscribeSentinel& subscribe_sentinel,
        std::vector<std::string> patterns,
        const CommandControl& command_control
    );

    template <typename T = Item>
    requires std::is_same_v<T, ShardedSubscriptionQueueItem>
    TokenType GetSubscriptionToken(
        impl::SubscribeSentinel& subscribe_sentinel,
        std::vector<std::string> channels,
        const CommandControl& command_control
    );

    // Messages could come out-of-order due to Redis limitations. Non FIFO is fine
    using Queue = concurrent::NonFifoMpscQueue<Item>;
    using Outcome = impl::Sentinel::Outcome;

    std::shared_ptr<Queue> queue_;
    typename Queue::Producer producer_;
    typename Queue::Consumer consumer_;
    MultiToken token_;
};

extern template class SubscriptionQueue<ChannelSubscriptionQueueItem>;
extern template class SubscriptionQueue<PatternSubscriptionQueueItem>;
extern template class SubscriptionQueue<ShardedSubscriptionQueueItem>;

}  // namespace storages::redis

USERVER_NAMESPACE_END
