#pragma once

#include <functional>
#include <memory>
#include <string>

namespace redis {
struct CommandControl;
class SubscribeSentinel;
}  // namespace redis

namespace storages {
namespace redis {

class SubscriptionTokenImplBase;

class SubscriptionToken {
 public:
  using OnMessageCb = std::function<void(const std::string& channel,
                                         const std::string& message)>;
  using OnPmessageCb =
      std::function<void(const std::string& pattern, const std::string& channel,
                         const std::string& message)>;

  SubscriptionToken();
  SubscriptionToken(::redis::SubscribeSentinel& subscribe_sentinel,
                    std::string channel, OnMessageCb on_message_cb,
                    const ::redis::CommandControl& command_control);
  SubscriptionToken(::redis::SubscribeSentinel& subscribe_sentinel,
                    std::string pattern, OnPmessageCb on_pmessage_cb,
                    const ::redis::CommandControl& command_control);
  ~SubscriptionToken();

  SubscriptionToken& operator=(SubscriptionToken&&);

  void SetMaxQueueLength(size_t length);

  void Unsubscribe();

 private:
  std::unique_ptr<SubscriptionTokenImplBase> impl_;
};

}  // namespace redis
}  // namespace storages
