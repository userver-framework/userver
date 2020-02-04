#pragma once

#include <memory>

#include <engine/single_consumer_event.hpp>

namespace utils::impl {

// Gives out tokens and waits for all given-out tokens death
class WaitTokenStorage final {
 public:
  WaitTokenStorage();

  WaitTokenStorage(const WaitTokenStorage&) = delete;
  WaitTokenStorage(WaitTokenStorage&&) = delete;

  class SafeScopeGuard;

  using Token = std::shared_ptr<SafeScopeGuard>;

  Token GetToken();

  /// Approximate number of currently alive tokens or -1 if storage is finalized
  long AliveTokensApprox() const;

  /// Wait until all given-out tokens are dead
  void WaitForAllTokens();

 private:
  std::shared_ptr<engine::SingleConsumerEvent> event_;
  Token token_;
};

}  // namespace utils::impl
