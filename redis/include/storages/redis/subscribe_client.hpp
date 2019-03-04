#pragma once

/// @file storages/redis/subscribe_client.hpp
/// @brief @copybrief storages::redis::SubscribeClient

#include <memory>
#include <string>

#include <redis/base.hpp>

#include <storages/redis/subscription_token.hpp>

namespace redis {
class SubscribeSentinel;
}  // namespace redis

namespace storages {
namespace redis {

/// @class SubscribeClient
/// @brief When you call `Subscribe()` or `Psubscribe()` command a new async
/// task will be started.
/// Callbacks will be called in this task strictly sequentially for each
/// received message.
/// You may call `utils::Async()` in the callback if you need parallel
/// message processing.
/// @note Messages can be received in any order due to redis sharding.
/// Sometimes messages can be duplicated due to subscriptions rebalancing.
/// Some messages may be lost (it's a redis limitation).
/// @note The first callback execution can happen before `Subscribe()` or
/// `Psubscribe()` return as it happens in a separate task.
class SubscribeClient {
 public:
  explicit SubscribeClient(
      std::shared_ptr<::redis::SubscribeSentinel> subscribe_sentinel);

  SubscriptionToken Subscribe(
      std::string channel, SubscriptionToken::OnMessageCb on_message_cb,
      const ::redis::CommandControl& command_control = {});

  SubscriptionToken Psubscribe(
      std::string pattern, SubscriptionToken::OnPmessageCb on_pmessage_cb,
      const ::redis::CommandControl& command_control = {});

  // For internal usage, don't use it
  ::redis::SubscribeSentinel& GetNative() const;

 private:
  std::shared_ptr<::redis::SubscribeSentinel> redis_client_;
};

}  // namespace redis
}  // namespace storages
