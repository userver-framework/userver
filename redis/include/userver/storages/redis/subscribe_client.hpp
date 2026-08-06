#pragma once

/// @file userver/storages/redis/subscribe_client.hpp
/// @brief @copybrief storages::redis::SubscribeClient

#include <string>

#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/client_fwd.hpp>
#include <userver/storages/redis/health_check_param.hpp>

#include <userver/storages/redis/subscription_token.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @ingroup userver_clients
///
/// @brief Client that allows subscribing to Redis channel messages.
///
/// Usually retrieved from components::Redis component.
///
/// Callback for each received message is called only
/// after the execution of callback for previous message has finished. For
/// parallel message processing use the utils::Async().
///
/// @warning Messages can be received in any order due to Redis sharding.
/// Sometimes messages can be duplicated due to subscriptions rebalancing.
/// Some messages may be lost (it's a Redis limitation).
///
/// @warning The first callback execution can happen before `Subscribe()` or
/// `Psubscribe()` return as it happens in a separate task.
///
/// @note a good GMock-based mock for this class can be found here:
/// userver/storages/redis/mock_subscribe_client.hpp
class SubscribeClient {
public:
    virtual ~SubscribeClient();

    virtual SubscriptionToken Subscribe(
        std::string channel,
        SubscriptionToken::OnMessageCb on_message_cb,
        const CommandControl& command_control
    ) = 0;

    SubscriptionToken Subscribe(std::string channel, SubscriptionToken::OnMessageCb on_message_cb) {
        return Subscribe(std::move(channel), std::move(on_message_cb), {});
    }

    virtual SubscriptionToken Psubscribe(
        std::string pattern,
        SubscriptionToken::OnPmessageCb on_pmessage_cb,
        const CommandControl& command_control
    ) = 0;

    virtual size_t ShardsCount() const = 0;
    virtual bool IsInClusterMode() const = 0;
    virtual bool IsReady(const HealthCheckParams& params) const = 0;

    bool IsReady(WaitConnectedMode mode) const { return IsReady(HealthCheckParams{mode, 0, 0}); }

    SubscriptionToken Psubscribe(std::string pattern, SubscriptionToken::OnPmessageCb on_pmessage_cb) {
        return Psubscribe(std::move(pattern), std::move(on_pmessage_cb), {});
    }

    virtual SubscriptionToken Ssubscribe(
        std::string channel,
        SubscriptionToken::OnMessageCb on_message_cb,
        const CommandControl& command_control
    ) = 0;

    /// Retrieves a single subscription token for multiple channels.
    /// Uses a single coroutine to handle messages from all channels.
    virtual SubscriptionToken Subscribe(
        std::vector<std::string> channels,
        SubscriptionToken::OnMessageCb on_message_cb,
        const CommandControl& command_control
    ) = 0;
    /// Retrieves a single subscription token for multiple channels.
    /// Uses a single coroutine to handle messages from all channels.
    virtual SubscriptionToken Psubscribe(
        std::vector<std::string> patterns,
        SubscriptionToken::OnPmessageCb on_message_cb,
        const CommandControl& command_control
    ) = 0;
    /// Retrieves a single subscription token for multiple channels.
    /// Uses a single coroutine to handle messages from all channels.
    virtual SubscriptionToken Ssubscribe(
        std::vector<std::string> channels,
        SubscriptionToken::OnMessageCb on_message_cb,
        const CommandControl& command_control
    ) = 0;

    SubscriptionToken Ssubscribe(std::string channel, SubscriptionToken::OnMessageCb on_message_cb) {
        return Ssubscribe(std::move(channel), std::move(on_message_cb), {});
    }

    SubscriptionToken Ssubscribe(std::vector<std::string> channels, SubscriptionToken::OnMessageCb on_message_cb) {
        return Ssubscribe(std::move(channels), std::move(on_message_cb), {});
    }

    SubscriptionToken Psubscribe(std::vector<std::string> patterns, SubscriptionToken::OnPmessageCb on_message_cb) {
        return Psubscribe(std::move(patterns), std::move(on_message_cb), {});
    }
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
