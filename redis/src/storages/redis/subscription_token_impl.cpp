#include "subscription_token_impl.hpp"

#include <stdexcept>

#include <userver/engine/task/task_with_result.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
namespace {

constexpr std::string_view kProcessRedisSubscriptionMessage =
    "process redis subscription message";

}  // namespace

SubscriptionTokenImpl::SubscriptionTokenImpl(
    USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
    std::string channel, OnMessageCb on_message_cb,
    const USERVER_NAMESPACE::redis::CommandControl& command_control)
    : channel_(std::move(channel)),
      queue_(subscribe_sentinel, channel_, command_control),
      on_message_cb_(std::move(on_message_cb)),
      subscriber_task_(
          utils::CriticalAsync("redis-channel-subscriber-" + channel_,
                               [this] { ProcessMessages(); })) {}

SubscriptionTokenImpl::~SubscriptionTokenImpl() { Unsubscribe(); }

void SubscriptionTokenImpl::SetMaxQueueLength(size_t length) {
  queue_.SetMaxLength(length);
}

void SubscriptionTokenImpl::Unsubscribe() {
  queue_.Unsubscribe();
  subscriber_task_.SyncCancel();
}

void SubscriptionTokenImpl::ProcessMessages() {
  ChannelSubscriptionQueueItem msg;
  while (queue_.PopMessage(msg)) {
    tracing::Span span(std::string{kProcessRedisSubscriptionMessage});
    if (on_message_cb_) on_message_cb_(channel_, msg.message);
  }
}

PsubscriptionTokenImpl::PsubscriptionTokenImpl(
    USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
    std::string pattern, OnPmessageCb on_pmessage_cb,
    const USERVER_NAMESPACE::redis::CommandControl& command_control)
    : pattern_(std::move(pattern)),
      queue_(subscribe_sentinel, pattern_, command_control),
      on_pmessage_cb_(std::move(on_pmessage_cb)),
      subscriber_task_(
          utils::CriticalAsync("redis-pattern-subscriber-" + pattern_,
                               [this] { ProcessMessages(); })) {}

PsubscriptionTokenImpl::~PsubscriptionTokenImpl() { Unsubscribe(); }

void PsubscriptionTokenImpl::SetMaxQueueLength(size_t length) {
  queue_.SetMaxLength(length);
}

void PsubscriptionTokenImpl::Unsubscribe() {
  queue_.Unsubscribe();
  subscriber_task_.SyncCancel();
}

void PsubscriptionTokenImpl::ProcessMessages() {
  PatternSubscriptionQueueItem msg;
  while (queue_.PopMessage(msg)) {
    tracing::Span span(std::string{kProcessRedisSubscriptionMessage});
    if (on_pmessage_cb_) on_pmessage_cb_(pattern_, msg.channel, msg.message);
  }
}

SsubscriptionTokenImpl::SsubscriptionTokenImpl(
    USERVER_NAMESPACE::redis::SubscribeSentinel& subscribe_sentinel,
    std::string channel, OnMessageCb on_message_cb,
    const USERVER_NAMESPACE::redis::CommandControl& command_control)
    : channel_(std::move(channel)),
      queue_(subscribe_sentinel, channel_, command_control),
      on_message_cb_(std::move(on_message_cb)),
      subscriber_task_(
          utils::CriticalAsync("redis-channel-subscriber-" + channel_,
                               [this] { ProcessMessages(); })) {}

SsubscriptionTokenImpl::~SsubscriptionTokenImpl() { Unsubscribe(); }

void SsubscriptionTokenImpl::SetMaxQueueLength(size_t length) {
  queue_.SetMaxLength(length);
}

void SsubscriptionTokenImpl::Unsubscribe() {
  queue_.Unsubscribe();
  subscriber_task_.SyncCancel();
}

void SsubscriptionTokenImpl::ProcessMessages() {
  ShardedSubscriptionQueueItem msg;
  while (queue_.PopMessage(msg)) {
    tracing::Span span(std::string{kProcessRedisSubscriptionMessage});
    if (on_message_cb_) on_message_cb_(channel_, msg.message);
  }
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
