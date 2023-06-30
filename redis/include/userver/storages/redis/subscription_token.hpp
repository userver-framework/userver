#pragma once

#include <functional>
#include <memory>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace redis {
struct CommandControl;
class SubscribeSentinel;
}  // namespace redis

namespace storages::redis {

/// You can inherit from this class to provide your own subscription
/// token implementation. Although it is useful only in mocks and tests.
/// All standard implementations for redis and so on are provided by userver and
/// there is no need to manually create instances of this interface.
/// A good GMock-based mock is here:
/// userver/storages/redis/mock_subscribe_client.hpp
class SubscriptionTokenImplBase {
 public:
  virtual ~SubscriptionTokenImplBase() = default;

  virtual void SetMaxQueueLength(size_t length) = 0;

  virtual void Unsubscribe() = 0;
};

class SubscriptionToken {
 public:
  using OnMessageCb = std::function<void(const std::string& channel,
                                         const std::string& message)>;
  using OnPmessageCb =
      std::function<void(const std::string& pattern, const std::string& channel,
                         const std::string& message)>;

  SubscriptionToken();
  SubscriptionToken(SubscriptionToken&&) noexcept;

  /// This constructor can be used in tests/mocks to create your own
  /// subscription token
  SubscriptionToken(
      std::unique_ptr<SubscriptionTokenImplBase>&& implementation);

  ~SubscriptionToken();

  SubscriptionToken& operator=(SubscriptionToken&&) noexcept;

  /// There is a MPSC queue inside the connection. This parameter requlates
  /// its maximum length. If it overflows, new messages are discarded.
  void SetMaxQueueLength(size_t length);

  /// Unsubscribe from the channel. This method is synchronous, once it
  /// returned, no new calls to callback will be made.
  void Unsubscribe();

  /// Checks that token is not empty. Empty token has no implementation
  /// inside. This method is mostly useful in unit tests. All methods
  /// of this class works correctly on empty tokens.
  bool IsEmpty() const noexcept { return impl_ == nullptr; }

 private:
  std::unique_ptr<SubscriptionTokenImplBase> impl_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
