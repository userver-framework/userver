#pragma once

#include <atomic>
#include <cstdint>

#include <userver/engine/single_use_event.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

// Gives out tokens and waits for all given-out tokens death
class WaitTokenStorage final {
 public:
  class Token final {
   public:
    Token(Token&&) noexcept;
    Token(const Token&) noexcept;
    Token& operator=(Token&&) noexcept;
    Token& operator=(const Token&) noexcept;
    ~Token();

    /// For internal use only
    explicit Token(WaitTokenStorage& storage) noexcept;

   private:
    WaitTokenStorage* storage_;
  };

  WaitTokenStorage();

  WaitTokenStorage(const WaitTokenStorage&) = delete;
  WaitTokenStorage(WaitTokenStorage&&) = delete;

  Token GetToken();

  /// Approximate number of currently alive tokens or -1 if storage is finalized
  std::int64_t AliveTokensApprox() const;

  /// Wait until all given-out tokens are dead
  void WaitForAllTokens() noexcept;

 private:
  std::atomic<std::int64_t> tokens_{1};
  engine::SingleUseEvent event_;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
