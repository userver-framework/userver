#include <userver/utils/token_bucket.hpp>

#include <stdexcept>

#include <userver/utils/datetime.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

void SetMinAtomic(std::atomic<size_t>& dest, size_t value) {
  size_t old = dest.load();
  while (old > value) {
    dest.compare_exchange_weak(old, value);
  }
}

constexpr TokenBucket::RefillPolicy kInstantRefillPolicy{
    1, TokenBucket::Duration::zero()};

}  // namespace

TokenBucket::TokenBucket() noexcept
    : max_size_(0),
      token_refill_amount_(0),
      token_refill_interval_(Duration::zero()),
      tokens_(0),
      last_update_(TimePoint{}) {
  static_assert(decltype(TokenBucket::max_size_)::is_always_lock_free);
  static_assert(
      decltype(TokenBucket::token_refill_interval_)::is_always_lock_free);
  static_assert(
      decltype(TokenBucket::token_refill_amount_)::is_always_lock_free);
  static_assert(decltype(TokenBucket::tokens_)::is_always_lock_free);
  static_assert(decltype(TokenBucket::last_update_)::is_always_lock_free);
}

TokenBucket::TokenBucket(size_t max_size, RefillPolicy policy) : TokenBucket() {
  SetMaxSize(max_size);
  SetRefillPolicy(policy);
}

TokenBucket::TokenBucket(size_t max_size, Duration single_token_update_interval)
    : TokenBucket(max_size, RefillPolicy{1, single_token_update_interval}) {}

TokenBucket TokenBucket::MakeUnbounded() noexcept {
  return TokenBucket{-1UL, kInstantRefillPolicy};
}

TokenBucket::TokenBucket(TokenBucket&& other) noexcept
    : max_size_(other.max_size_.load()),
      token_refill_amount_(other.token_refill_amount_.load()),
      token_refill_interval_(other.token_refill_interval_.load()),
      tokens_(other.tokens_.load()),
      last_update_(other.last_update_.load()) {}

TokenBucket& TokenBucket::operator=(TokenBucket&& rhs) noexcept {
  max_size_ = rhs.max_size_.load();
  token_refill_amount_ = rhs.token_refill_amount_.load();
  token_refill_interval_ = rhs.token_refill_interval_.load();
  tokens_ = rhs.tokens_.load();
  last_update_ = rhs.last_update_.load();
  return *this;
}

bool TokenBucket::IsUnbounded() const {
  return token_refill_amount_ == kInstantRefillPolicy.amount &&
         token_refill_interval_.load() == kInstantRefillPolicy.interval;
}

size_t TokenBucket::GetMaxSizeApprox() const { return max_size_.load(); }

TokenBucket::Duration TokenBucket::GetRefillIntervalApprox() const {
  return token_refill_interval_.load();
}

size_t TokenBucket::GetRefillAmountApprox() const {
  return token_refill_amount_.load();
}

double TokenBucket::GetRatePs() const {
  return GetRatePs(GetRefillIntervalApprox()) * GetRefillAmountApprox();
}

size_t TokenBucket::GetTokensApprox() {
  Update();
  return tokens_.load();
}

void TokenBucket::SetMaxSize(size_t max_size) {
  max_size_ = max_size;
  if (token_refill_interval_.load().count() > 0) {
    SetMinAtomic(tokens_, max_size);
  } else {
    tokens_ = max_size;
  }
}

void TokenBucket::SetUpdateInterval(Duration single_token_update_interval) {
  SetRefillPolicy({1, single_token_update_interval});
}

void TokenBucket::SetRefillPolicy(RefillPolicy policy) {
  if (policy.interval.count() < 0) {
    throw std::runtime_error(
        "TokenBucket refill interval must be non-negative");
  }
  token_refill_amount_ = policy.amount;
  token_refill_interval_ = policy.interval;
  if (policy.interval.count() == 0) {
    tokens_ = max_size_.load();
  }
}

void TokenBucket::SetInstantRefillPolicy() {
  SetRefillPolicy(kInstantRefillPolicy);
}

bool TokenBucket::Obtain() { return ObtainAll(1); }

bool TokenBucket::ObtainAll(size_t count) {
  if (max_size_ < count) {
    // not satisfiable
    return false;
  }

  Update();

  if (!count || (token_refill_interval_.load().count() == 0 &&
                 token_refill_amount_.load() != 0)) {
    // no need to subtract
    return true;
  }

  auto expected = tokens_.load();
  do {
    if (expected < count) return false;
  } while (!tokens_.compare_exchange_strong(expected, expected - count));

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
  const auto max_size = max_size_.load();
  const auto token_update_amount = token_refill_amount_.load();
  const auto token_update_interval = token_refill_interval_.load();
  if (max_size == 0 || token_update_amount == 0 ||
      token_update_interval.count() == 0)
    return;

  const auto update_tp = last_update_.load();
  const auto now = utils::datetime::MockSteadyNow();
  if (now < update_tp) {
    last_update_ = now;  // overflow
    return;
  }
  if (now == update_tp) return;

  auto update_ticks = (now - update_tp) / token_update_interval;
  if (update_ticks == 0) return;

  const auto tokens = tokens_.load();

  // Possible while updating max_size
  const auto space_left = max_size > tokens ? max_size - tokens : 0;
  size_t tokens_to_add = 0;

  TimePoint new_update_tp;
  if (token_update_amount > space_left / update_ticks) {
    tokens_to_add = space_left;
    new_update_tp = now;
  } else {
    tokens_to_add = token_update_amount * update_ticks;
    new_update_tp = update_tp + token_update_interval * update_ticks;
  }

  auto tmp = update_tp;
  if (last_update_.compare_exchange_strong(tmp, new_update_tp)) {
    // We've committed to add 'tokens_to_add'
    tokens_ += tokens_to_add;
  } else {
    // Someone has updated the counter just now.
    // Do not retry as there is no token updates yet.
  }
}

}  // namespace utils

USERVER_NAMESPACE_END
