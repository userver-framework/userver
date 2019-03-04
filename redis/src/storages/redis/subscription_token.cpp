#include <storages/redis/subscription_token.hpp>

#include <stdexcept>

#include <engine/task/task_with_result.hpp>
#include <tracing/span.hpp>
#include <utils/async.hpp>

#include "subscription_queue.hpp"

namespace storages {
namespace redis {
namespace {

const std::string kSubscribeToChannelPrefix = "redis-channel-subscriber-";
const std::string kSubscribeToPatternPrefix = "redis-pattern-subscriber-";
const std::string kProcessRedisSubscriptionMessage =
    "process redis subscription message";

}  // namespace

class SubscriptionTokenImplBase {
 public:
  virtual ~SubscriptionTokenImplBase() {}

  virtual void SetMaxQueueLength(size_t length) = 0;

  virtual void Unsubscribe() = 0;
};

class SubscriptionTokenImpl : public SubscriptionTokenImplBase {
 public:
  using OnMessageCb = SubscriptionToken::OnMessageCb;

  SubscriptionTokenImpl(::redis::SubscribeSentinel& subscribe_sentinel,
                        std::string channel, OnMessageCb on_message_cb,
                        const ::redis::CommandControl& command_control);

  ~SubscriptionTokenImpl();

  void SetMaxQueueLength(size_t length) override;

  void Unsubscribe() override;

 private:
  void ProcessMessages();

  std::string channel_;
  std::unique_ptr<SubscriptionQueue<ChannelSubscriptionQueueItem>> queue_;
  OnMessageCb on_message_cb_;
  engine::TaskWithResult<void> subscriber_task_;
};

SubscriptionTokenImpl::SubscriptionTokenImpl(
    ::redis::SubscribeSentinel& subscribe_sentinel, std::string channel,
    OnMessageCb on_message_cb, const ::redis::CommandControl& command_control)
    : channel_(std::move(channel)),
      queue_(std::make_unique<SubscriptionQueue<ChannelSubscriptionQueueItem>>(
          subscribe_sentinel, channel_, command_control)),
      on_message_cb_(std::move(on_message_cb)),
      subscriber_task_(utils::Async(kSubscribeToChannelPrefix + channel_,
                                    [this] { ProcessMessages(); })) {}

SubscriptionTokenImpl::~SubscriptionTokenImpl() { Unsubscribe(); }

void SubscriptionTokenImpl::SetMaxQueueLength(size_t length) {
  queue_->SetMaxLength(length);
}

void SubscriptionTokenImpl::Unsubscribe() {
  queue_->Unsubscribe();
  subscriber_task_.RequestCancel();
  subscriber_task_.Wait();
}

void SubscriptionTokenImpl::ProcessMessages() {
  std::unique_ptr<ChannelSubscriptionQueueItem> msg;
  while (queue_->PopMessage(msg)) {
    tracing::Span span(kProcessRedisSubscriptionMessage);
    if (on_message_cb_) on_message_cb_(channel_, msg->message);
  }
}

class PsubscriptionTokenImpl : public SubscriptionTokenImplBase {
 public:
  using OnPmessageCb = SubscriptionToken::OnPmessageCb;

  PsubscriptionTokenImpl(::redis::SubscribeSentinel& subscribe_sentinel,
                         std::string pattern, OnPmessageCb on_pmessage_cb,
                         const ::redis::CommandControl& command_control);

  ~PsubscriptionTokenImpl();

  void SetMaxQueueLength(size_t length) override;

  void Unsubscribe() override;

 private:
  void ProcessMessages();

  std::string pattern_;
  std::unique_ptr<SubscriptionQueue<PatternSubscriptionQueueItem>> queue_;
  OnPmessageCb on_pmessage_cb_;
  engine::TaskWithResult<void> subscriber_task_;
};

PsubscriptionTokenImpl::PsubscriptionTokenImpl(
    ::redis::SubscribeSentinel& subscribe_sentinel, std::string pattern,
    OnPmessageCb on_pmessage_cb, const ::redis::CommandControl& command_control)
    : pattern_(std::move(pattern)),
      queue_(std::make_unique<SubscriptionQueue<PatternSubscriptionQueueItem>>(
          subscribe_sentinel, pattern_, command_control)),
      on_pmessage_cb_(std::move(on_pmessage_cb)),
      subscriber_task_(utils::Async(kSubscribeToPatternPrefix + pattern_,
                                    [this] { ProcessMessages(); })) {}

PsubscriptionTokenImpl::~PsubscriptionTokenImpl() { Unsubscribe(); }

void PsubscriptionTokenImpl::SetMaxQueueLength(size_t length) {
  queue_->SetMaxLength(length);
}

void PsubscriptionTokenImpl::Unsubscribe() {
  queue_->Unsubscribe();
  subscriber_task_.RequestCancel();
  subscriber_task_.Wait();
}

void PsubscriptionTokenImpl::ProcessMessages() {
  std::unique_ptr<PatternSubscriptionQueueItem> msg;
  while (queue_->PopMessage(msg)) {
    tracing::Span span(kProcessRedisSubscriptionMessage);
    if (on_pmessage_cb_) on_pmessage_cb_(pattern_, msg->channel, msg->message);
  }
}

SubscriptionToken::SubscriptionToken() = default;

SubscriptionToken::SubscriptionToken(
    ::redis::SubscribeSentinel& subscribe_sentinel, std::string channel,
    OnMessageCb on_message_cb, const ::redis::CommandControl& command_control)
    : impl_(std::make_unique<SubscriptionTokenImpl>(
          subscribe_sentinel, std::move(channel), std::move(on_message_cb),
          command_control)) {}

SubscriptionToken::SubscriptionToken(
    ::redis::SubscribeSentinel& subscribe_sentinel, std::string pattern,
    OnPmessageCb on_pmessage_cb, const ::redis::CommandControl& command_control)
    : impl_(std::make_unique<PsubscriptionTokenImpl>(
          subscribe_sentinel, std::move(pattern), std::move(on_pmessage_cb),
          command_control)) {}

SubscriptionToken::~SubscriptionToken() {}

SubscriptionToken& SubscriptionToken::operator=(SubscriptionToken&&) = default;

void SubscriptionToken::SetMaxQueueLength(size_t length) {
  if (!impl_) throw std::runtime_error("SubscriptionToken is not initialized");
  impl_->SetMaxQueueLength(length);
}

void SubscriptionToken::Unsubscribe() {
  if (!impl_) return;
  impl_->Unsubscribe();
}

}  // namespace redis
}  // namespace storages
