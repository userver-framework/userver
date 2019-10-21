#pragma once

#include <memory>

#include <engine/single_consumer_event.hpp>

namespace rcu::impl {

// Gives out tokens and waits for all given-out tokens death
class WaitTokenStorage final {
 public:
  WaitTokenStorage();

  WaitTokenStorage(const WaitTokenStorage&) = delete;
  WaitTokenStorage(WaitTokenStorage&&) = delete;

  class SafeScopeGuard;

  using Token = std::shared_ptr<SafeScopeGuard>;

  Token GetToken();

  /// Wait until all given-out tokens are dead
  void WaitForAllTokens();

 private:
  std::shared_ptr<engine::SingleConsumerEvent> event_;
  Token token_;
};

}  // namespace rcu::impl
