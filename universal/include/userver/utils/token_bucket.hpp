#pragma once

/// @file userver/utils/token_bucket.hpp
/// @brief @copybrief utils::TokenBucket

#include <atomic>
#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_concurrency
///
/// Thread safe ratelimiter
class TokenBucket final {
 public:
  using TimePoint = std::chrono::steady_clock::time_point;
  using Duration = std::chrono::steady_clock::duration;

  /// Token bucket refill policy
  struct RefillPolicy {
    /// Refill amount (zero disables refills)
    size_t amount{1};
    /// Refill interval (zero makes bucket to instantly refill)
    Duration interval{Duration::max()};
  };

  /// Create an initially always empty token bucket
  TokenBucket() noexcept;

  /// Create a token bucket with max_size tokens and a specified refill policy
  TokenBucket(size_t max_size, RefillPolicy policy);

  /// Start with max_size tokens and add 1 token each
  /// single_token_update_interval up to max_size.
  /// Zero duration means "no limit".
  [[deprecated]] TokenBucket(size_t max_size,
                             Duration single_token_update_interval);

  /// Create an initially unbounded token bucket (largest size, instant refill)
  static TokenBucket MakeUnbounded() noexcept;

  TokenBucket(const TokenBucket&) = delete;
  TokenBucket(TokenBucket&&) noexcept;
  TokenBucket& operator=(const TokenBucket&) = delete;
  TokenBucket& operator=(TokenBucket&&) noexcept;

  bool IsUnbounded() const;

  /// Get current token limit (might be inaccurate as the result is stale)
  size_t GetMaxSizeApprox() const;

  /// Get current refill amount (might be inaccurate as the result is stale)
  size_t GetRefillAmountApprox() const;

  /// Get current refill interval (might be inaccurate as the result is stale)
  Duration GetRefillIntervalApprox() const;

  /// Get rate (tokens per second)
  double GetRatePs() const;

  /// Get current token count (might be inaccurate as the result is stale)
  size_t GetTokensApprox();

  /// Set max token count
  void SetMaxSize(size_t max_size);

  /// Add 1 token each token_update_interval.
  /// Zero duration means "no limit".
  [[deprecated]] void SetUpdateInterval(Duration single_token_update_interval);

  /// Set refill policy for the bucket
  void SetRefillPolicy(RefillPolicy policy);

  /// Set refill policy to "instant refill".
  ///
  /// Obtain does not deplete the bucket in this mode.
  /// Equivalent to `amount=1, interval=zero()` policy.
  void SetInstantRefillPolicy();

  /// @returns true if token was successfully obtained
  [[nodiscard]] bool Obtain();

  /// @return true if the requested number of tokens was successfully obtained
  [[nodiscard]] bool ObtainAll(size_t count);

  /// Get rate for specified update interval (updates per second)
  static double GetRatePs(Duration interval);

 private:
  void Update();

  std::atomic<size_t> max_size_;
  std::atomic<size_t> token_refill_amount_;
  std::atomic<Duration> token_refill_interval_;
  std::atomic<size_t> tokens_;
  std::atomic<TimePoint> last_update_;
};

}  // namespace utils

USERVER_NAMESPACE_END
