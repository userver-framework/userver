#include <storages/redis/subscribe_client.hpp>

namespace storages {
namespace redis {

SubscribeClient::SubscribeClient(
    std::shared_ptr<::redis::SubscribeSentinel> subscribe_sentinel)
    : redis_client_(std::move(subscribe_sentinel)) {}

SubscriptionToken SubscribeClient::Subscribe(
    std::string channel, SubscriptionToken::OnMessageCb on_message_cb,
    const ::redis::CommandControl& command_control) {
  return {*redis_client_, std::move(channel), std::move(on_message_cb),
          command_control};
}

SubscriptionToken SubscribeClient::Psubscribe(
    std::string pattern, SubscriptionToken::OnPmessageCb on_pmessage_cb,
    const ::redis::CommandControl& command_control) {
  return {*redis_client_, std::move(pattern), std::move(on_pmessage_cb),
          command_control};
}

::redis::SubscribeSentinel& SubscribeClient::GetNative() const {
  return *redis_client_;
}

}  // namespace redis
}  // namespace storages
