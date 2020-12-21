#include <utils/token_bucket.hpp>

#include <stdexcept>

#include <utils/mock_now.hpp>

namespace utils {

namespace {
void SetMinAtomic(std::atomic<size_t>& dest, size_t value) {
  size_t old = dest.load();
  while (old > value) {
    dest.compare_exchange_weak(old, value);
  }
}
}  // namespace

TokenBucket::TokenBucket(size_t max_size, Duration single_token_update_interval)
    : max_size_(max_size),
      single_token_update_interval_(std::chrono::seconds(1) /* not used */),
      tokens_(max_size),
      last_update_tp_(GetNow()) {
  SetUpdateInterval(single_token_update_interval);
  SetMaxSize(max_size);
}

TokenBucket::TokenBucket(TokenBucket&& other) noexcept
    : max_size_(other.max_size_.load()),
      single_token_update_interval_(other.single_token_update_interval_.load()),
      tokens_(other.tokens_.load()),
      last_update_tp_(other.last_update_tp_.load()) {}

TokenBucket& TokenBucket::operator=(TokenBucket&& rhs) noexcept {
  max_size_ = rhs.max_size_.load();
  single_token_update_interval_ = rhs.single_token_update_interval_.load();
  tokens_ = rhs.tokens_.load();
  last_update_tp_ = rhs.last_update_tp_.load();
  return *this;
}

void TokenBucket::SetMaxSize(size_t max_size) {
  if (max_size == 0) {
    throw std::runtime_error("TokenBucket max_size must be positive");
  }
  max_size_ = max_size;
  SetMinAtomic(tokens_, max_size);
}

void TokenBucket::SetUpdateInterval(Duration single_token_update_interval) {
  if (single_token_update_interval.count() < 0) {
    throw std::runtime_error(
        "TokenBucket token_update_interval must be non-negative");
  }
  single_token_update_interval_ = single_token_update_interval;
}

void TokenBucket::Drain() {
  tokens_ = 0;
  last_update_tp_ = GetNow();
}

double TokenBucket::GetRatePs() const {
  return GetRatePs(
      single_token_update_interval_.load(std::memory_order_relaxed));
}

size_t TokenBucket::GetTokensApprox() const { return tokens_.load(); }

bool TokenBucket::Obtain() {
  Update();

  if (single_token_update_interval_.load().count() == 0) {
    // No limit
    return true;
  }

  auto expected = tokens_.load();
  do {
    if (expected == 0) return false;
  } while (!tokens_.compare_exchange_strong(expected, expected - 1));

  return true;
}

double TokenBucket::GetRatePs(Duration update_interval) {
  if (update_interval.count())
    return 1.0 / update_interval.count() / Duration::period::num *
           Duration::period::den;
  else
    return 0;
}

void TokenBucket::Update() {
  auto update_tp = last_update_tp_.load();
  const auto now = GetNow();
  if (now < update_tp) {
    last_update_tp_ = now;  // overflow
    return;
  }
  if (now == update_tp) return;

  const auto max_size = max_size_.load();
  const auto token_update_interval = single_token_update_interval_.load();
  if (token_update_interval.count() == 0) return;

  auto tokens_to_add = (now - update_tp) / token_update_interval;
  if (tokens_to_add == 0) return;

  const auto tokens = tokens_.load();

  Duration new_update_tp;
  if (tokens + tokens_to_add >= max_size) {
    new_update_tp = now;

    if (max_size < tokens) {
      // Possible while updating max_size
      tokens_to_add = 0;
    } else {
      tokens_to_add = max_size - tokens;
    }
  } else {
    new_update_tp = update_tp + token_update_interval * tokens_to_add;
  }

  if (last_update_tp_.compare_exchange_strong(update_tp, new_update_tp)) {
    // We've committed to add 'tokens_to_add'
    tokens_ += tokens_to_add;
  } else {
    // Someone has updated the counter just now.
    // Do not retry as there is no token updates yet.
  }
}

TokenBucket::Duration TokenBucket::GetNow() {
  return utils::datetime::MockSteadyNow().time_since_epoch();
}

}  // namespace utils
