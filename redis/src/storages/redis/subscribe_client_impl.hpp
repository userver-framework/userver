#pragma once

#include <memory>
#include <string>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/wait_connected_mode.hpp>

#include <userver/storages/redis/subscribe_client.hpp>
#include <userver/storages/redis/subscription_token.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
class SubscribeSentinel;
}  // namespace redis

namespace storages::redis {

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
class SubscribeClientImpl final : public SubscribeClient {
 public:
  explicit SubscribeClientImpl(
      std::shared_ptr<USERVER_NAMESPACE::redis::SubscribeSentinel>
          subscribe_sentinel);

  SubscriptionToken Subscribe(
      std::string channel, SubscriptionToken::OnMessageCb on_message_cb,
      const USERVER_NAMESPACE::redis::CommandControl& command_control) override;

  SubscriptionToken Psubscribe(
      std::string pattern, SubscriptionToken::OnPmessageCb on_pmessage_cb,
      const USERVER_NAMESPACE::redis::CommandControl& command_control) override;

  void WaitConnectedOnce(
      USERVER_NAMESPACE::redis::RedisWaitConnected wait_connected);

  // For internal usage, don't use it
  USERVER_NAMESPACE::redis::SubscribeSentinel& GetNative() const;

 private:
  std::shared_ptr<USERVER_NAMESPACE::redis::SubscribeSentinel> redis_client_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
