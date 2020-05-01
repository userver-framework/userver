#include "subscribe_client_impl.hpp"

#include <storages/redis/impl/subscribe_sentinel.hpp>
#include <storages/redis/subscription_token_impl.hpp>

namespace storages {
namespace redis {

SubscribeClientImpl::SubscribeClientImpl(
    std::shared_ptr<::redis::SubscribeSentinel> subscribe_sentinel)
    : redis_client_(std::move(subscribe_sentinel)) {}

SubscriptionToken SubscribeClientImpl::Subscribe(
    std::string channel, SubscriptionToken::OnMessageCb on_message_cb,
    const ::redis::CommandControl& command_control) {
  return {std::make_unique<SubscriptionTokenImpl>(
      *redis_client_, std::move(channel), std::move(on_message_cb),
      command_control)};
}

SubscriptionToken SubscribeClientImpl::Psubscribe(
    std::string pattern, SubscriptionToken::OnPmessageCb on_pmessage_cb,
    const ::redis::CommandControl& command_control) {
  return {std::make_unique<PsubscriptionTokenImpl>(
      *redis_client_, std::move(pattern), std::move(on_pmessage_cb),
      command_control)};
}

void SubscribeClientImpl::WaitConnectedOnce(
    ::redis::RedisWaitConnected wait_connected) {
  redis_client_->WaitConnectedOnce(wait_connected);
}

::redis::SubscribeSentinel& SubscribeClientImpl::GetNative() const {
  return *redis_client_;
}

}  // namespace redis
}  // namespace storages
