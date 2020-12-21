#pragma once

#include <atomic>
#include <chrono>

namespace utils {

/// Thread safe ratelimiter
class TokenBucket final {
 public:
  using Duration = std::chrono::steady_clock::duration;

  /// Start with max_size tokens and add 1 token each
  /// single_token_update_interval up to max_size.
  /// Zero duration means "no limit".
  TokenBucket(size_t max_size, Duration single_token_update_interval);

  TokenBucket(const TokenBucket&) = delete;
  TokenBucket(TokenBucket&&) noexcept;
  TokenBucket& operator=(const TokenBucket&) = delete;
  TokenBucket& operator=(TokenBucket&&) noexcept;

  /// @returns true if token was successfully obtained
  bool Obtain();

  /// Set max token count
  void SetMaxSize(size_t max_size);

  /// Add 1 token each token_update_interval.
  /// Zero duration means "no limit".
  void SetUpdateInterval(Duration single_token_update_interval);

  /// Get rate (tokens per second)
  double GetRatePs() const;

  /// Get current token count (might be inaccurate as the result is stale)
  size_t GetTokensApprox() const;

  /// Get rate for specified update interval (tokens per second)
  static double GetRatePs(Duration update_interval);

  /// Remove all existing tokens
  void Drain();

 private:
  void Update();

  static Duration GetNow();

  std::atomic<size_t> max_size_;
  std::atomic<Duration> single_token_update_interval_;
  std::atomic<size_t> tokens_;
  std::atomic<Duration> last_update_tp_;
};

}  // namespace utils
