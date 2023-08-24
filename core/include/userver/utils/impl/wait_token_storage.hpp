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
    Token() noexcept = default;
    Token(Token&&) noexcept;
    Token(const Token&) noexcept;
    Token& operator=(Token&&) noexcept;
    Token& operator=(const Token&) noexcept;
    ~Token();

    /// For internal use only
    explicit Token(WaitTokenStorage& storage) noexcept;

   private:
    WaitTokenStorage* storage_{nullptr};
  };

  WaitTokenStorage();

  WaitTokenStorage(const WaitTokenStorage&) = delete;
  WaitTokenStorage(WaitTokenStorage&&) = delete;
  ~WaitTokenStorage();

  Token GetToken();

  /// Approximate number of currently alive tokens
  std::uint64_t AliveTokensApprox() const noexcept;

  /// Wait until all given-out tokens are dead. Should be called at most once,
  /// either in a coroutine environment or after the coroutine environment
  /// stops (during static destruction).
  void WaitForAllTokens() noexcept;

 private:
  std::atomic<std::int64_t> tokens_;
  engine::SingleUseEvent event_;
};

}  // namespace utils::impl

USERVER_NAMESPACE_END
