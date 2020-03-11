#pragma once

#include <atomic>
#include <chrono>

namespace utils {

/// Thread safe ratelimit
class TokenBucket final {
 public:
  using Duration = std::chrono::steady_clock::duration;

  /// Start with max_size tokens and add 1 token each
  /// single_token_update_interval up to max_size.
  TokenBucket(size_t max_size, Duration single_token_update_interval);

  TokenBucket(const TokenBucket&) = delete;
  TokenBucket(TokenBucket&&) = delete;

  /// @returns true if token was successfully obtained
  bool Obtain();

  /// Set max token count
  void SetMaxSize(size_t max_size);

  /// Add 1 token each token_update_interval
  void SetUpdateInterval(Duration single_token_update_interval);

 private:
  void Update();

  static Duration GetNow();

  std::atomic<size_t> max_size_;
  std::atomic<Duration> single_token_update_interval_;
  std::atomic<size_t> tokens_;
  std::atomic<Duration> last_update_tp_;
};

}  // namespace utils
