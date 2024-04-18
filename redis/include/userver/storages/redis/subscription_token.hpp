#pragma once

/// @file userver/storages/redis/subscriptioin_token.hpp
/// @brief @copybrief storages::redis::SubscriptionToken

#include <functional>
#include <memory>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace redis {
struct CommandControl;
class SubscribeSentinel;
}  // namespace redis

namespace storages::redis {

namespace impl {

class SubscriptionTokenImplBase {
 public:
  SubscriptionTokenImplBase() = default;

  SubscriptionTokenImplBase(SubscriptionTokenImplBase&&) = delete;
  SubscriptionTokenImplBase& operator=(SubscriptionTokenImplBase&&) = delete;

  SubscriptionTokenImplBase(const SubscriptionTokenImplBase&) = delete;
  SubscriptionTokenImplBase& operator=(const SubscriptionTokenImplBase&&) =
      delete;

  virtual ~SubscriptionTokenImplBase();

  virtual void SetMaxQueueLength(size_t length) = 0;

  virtual void Unsubscribe() = 0;
};

}  // namespace impl

/// @brief RAII subscription guard, that is usually retrieved from
/// storages::redis::SubscribeClient.
///
/// See storages::redis::MockSubscriptionTokenImpl for hints on mocking and
/// testing.
class [[nodiscard]] SubscriptionToken final {
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
      std::unique_ptr<impl::SubscriptionTokenImplBase>&& implementation);

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
  /// of this class work correctly on empty tokens.
  bool IsEmpty() const noexcept { return impl_ == nullptr; }

 private:
  std::unique_ptr<impl::SubscriptionTokenImplBase> impl_;
};

}  // namespace storages::redis

USERVER_NAMESPACE_END
