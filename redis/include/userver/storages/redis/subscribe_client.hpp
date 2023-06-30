#pragma once

/// @file userver/storages/redis/subscribe_client.hpp
/// @brief @copybrief storages::redis::SubscribeClient

#include <memory>
#include <string>

#include <userver/storages/redis/client_fwd.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/wait_connected_mode.hpp>

#include <userver/storages/redis/subscription_token.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
class SubscribeSentinel;
}  // namespace redis

namespace storages::redis {

/// @class SubscribeClient
/// @brief When you call `Subscribe()` or `Psubscribe()` command in a queuing
/// mode (default mode), a new async task will be started. Queueing allows
/// for better error handling and prevents service OOM. However, if you
/// intend to implement such queue by yourself, you can disabled queueing
/// by setting appropriate flag in CommandControl structure.
///
/// Callbacks will be called in this task strictly sequentially for each
/// received message.
/// You may call `utils::Async()` in the callback if you need parallel
/// message processing.
/// @note Messages can be received in any order due to redis sharding.
/// Sometimes messages can be duplicated due to subscriptions rebalancing.
/// Some messages may be lost (it's a redis limitation).
/// @note The first callback execution can happen before `Subscribe()` or
/// `Psubscribe()` return as it happens in a separate task.
/// @note a good GMock-based mock for this class can be found here:
/// userver/storages/redis/mock_subscribe_client.hpp
class SubscribeClient {
 public:
  virtual ~SubscribeClient() = default;

  virtual SubscriptionToken Subscribe(
      std::string channel, SubscriptionToken::OnMessageCb on_message_cb,
      const USERVER_NAMESPACE::redis::CommandControl& command_control) = 0;

  SubscriptionToken Subscribe(std::string channel,
                              SubscriptionToken::OnMessageCb on_message_cb) {
    return Subscribe(std::move(channel), std::move(on_message_cb), {});
  }

  virtual SubscriptionToken Psubscribe(
      std::string pattern, SubscriptionToken::OnPmessageCb on_pmessage_cb,
      const USERVER_NAMESPACE::redis::CommandControl& command_control) = 0;

  SubscriptionToken Psubscribe(std::string pattern,
                               SubscriptionToken::OnPmessageCb on_pmessage_cb) {
    return Psubscribe(std::move(pattern), std::move(on_pmessage_cb), {});
  }
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
