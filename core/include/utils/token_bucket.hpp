#pragma once

#include <atomic>
#include <chrono>

namespace utils {

/// Thread safe ratelimit
class TokenBucket final {
 public:
  using Duration = std::chrono::steady_clock::duration;

  TokenBucket(size_t max_size, Duration token_update_interval);
  TokenBucket(const TokenBucket&) = delete;
  TokenBucket(TokenBucket&&) = delete;

  bool Obtain();

  void SetMaxSize(size_t max_size);

  void SetUpdateInterval(Duration token_update_interval);

 private:
  void Update();

  static Duration GetNow();

  std::atomic<size_t> max_size_;
  std::atomic<Duration> token_update_interval_;
  std::atomic<size_t> tokens_;
  std::atomic<Duration> last_update_tp_;
};

}  // namespace utils
