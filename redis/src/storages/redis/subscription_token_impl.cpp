#include "subscription_token_impl.hpp"

#include <stdexcept>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace {

constexpr std::string_view kProcessRedisSubscriptionMessage = "process redis subscription message";

}  // namespace

SubscriptionTokenImpl::SubscriptionTokenImpl(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channels,
    OnMessageCb on_message_cb,
    const CommandControl& command_control
)
    : queue_(subscribe_sentinel, channels, command_control),
      on_message_cb_(std::move(on_message_cb)),
      subscriber_task_(utils::CriticalAsync("redis-channel-subscriber-" + channels.at(0), [this] { ProcessMessages(); })
      )
{}

SubscriptionTokenImpl::~SubscriptionTokenImpl() { Unsubscribe(); }

void SubscriptionTokenImpl::SetMaxQueueLength(size_t length) { queue_.SetMaxLength(length); }

void SubscriptionTokenImpl::Unsubscribe() {
    queue_.Unsubscribe();
    subscriber_task_.SyncCancel();
}

void SubscriptionTokenImpl::ProcessMessages() {
    ChannelSubscriptionQueueItem msg;
    while (queue_.PopMessage(msg)) {
        const tracing::Span span(std::string{kProcessRedisSubscriptionMessage});
        if (on_message_cb_) {
            on_message_cb_(msg.channel, msg.message);
        }
    }
}

PsubscriptionTokenImpl::PsubscriptionTokenImpl(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> patterns,
    OnPmessageCb on_pmessage_cb,
    const CommandControl& command_control
)
    : queue_(subscribe_sentinel, patterns, command_control),
      on_pmessage_cb_(std::move(on_pmessage_cb)),
      subscriber_task_(utils::CriticalAsync("redis-pattern-subscriber-" + patterns.at(0), [this] { ProcessMessages(); })
      )
{}

PsubscriptionTokenImpl::~PsubscriptionTokenImpl() { Unsubscribe(); }

void PsubscriptionTokenImpl::SetMaxQueueLength(size_t length) { queue_.SetMaxLength(length); }

void PsubscriptionTokenImpl::Unsubscribe() {
    queue_.Unsubscribe();
    subscriber_task_.SyncCancel();
}

void PsubscriptionTokenImpl::ProcessMessages() {
    PatternSubscriptionQueueItem msg;
    while (queue_.PopMessage(msg)) {
        const tracing::Span span(std::string{kProcessRedisSubscriptionMessage});
        if (on_pmessage_cb_) {
            on_pmessage_cb_(msg.pattern, msg.channel, msg.message);
        }
    }
}

SsubscriptionTokenImpl::SsubscriptionTokenImpl(
    impl::SubscribeSentinel& subscribe_sentinel,
    std::vector<std::string> channels,
    OnMessageCb on_message_cb,
    const CommandControl& command_control
)
    : queue_(subscribe_sentinel, channels, command_control),
      on_message_cb_(std::move(on_message_cb)),
      subscriber_task_(utils::CriticalAsync("redis-channel-subscriber-" + channels.at(0), [this] { ProcessMessages(); })
      )
{}

SsubscriptionTokenImpl::~SsubscriptionTokenImpl() { Unsubscribe(); }

void SsubscriptionTokenImpl::SetMaxQueueLength(size_t length) { queue_.SetMaxLength(length); }

void SsubscriptionTokenImpl::Unsubscribe() {
    queue_.Unsubscribe();
    subscriber_task_.SyncCancel();
}

void SsubscriptionTokenImpl::ProcessMessages() {
    ShardedSubscriptionQueueItem msg;
    while (queue_.PopMessage(msg)) {
        const tracing::Span span(std::string{kProcessRedisSubscriptionMessage});
        if (on_message_cb_) {
            on_message_cb_(msg.channel, msg.message);
        }
    }
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
