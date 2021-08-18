#pragma once

#include <atomic>
#include <cstdint>
#include <memory>

#include <userver/engine/single_use_event.hpp>

namespace utils::impl {

// Gives out tokens and waits for all given-out tokens death
class WaitTokenStorage final {
 private:
  struct TokenDeleter final {
    void operator()(WaitTokenStorage* storage) noexcept;
  };

 public:
  WaitTokenStorage();

  WaitTokenStorage(const WaitTokenStorage&) = delete;
  WaitTokenStorage(WaitTokenStorage&&) = delete;

  using Token = std::unique_ptr<WaitTokenStorage, TokenDeleter>;

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
