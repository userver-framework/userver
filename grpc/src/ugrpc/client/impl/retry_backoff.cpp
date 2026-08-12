#include <userver/ugrpc/client/impl/retry_backoff.hpp>

#include <algorithm>

#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::impl {

namespace {

constexpr std::int64_t kInitialBackoffMs = 10;
constexpr std::int64_t kMaxBackoffMs = 300;

constexpr float kBackoffMultiplier = 2.0;

constexpr double kJitter = 0.2;

}  // namespace

RetryBackoff::RetryBackoff() { Reset(); }

std::chrono::milliseconds RetryBackoff::NextAttemptDelay() {
    if (initial_) {
        initial_ = false;
    } else {
        current_backoff_ms_ *= kBackoffMultiplier;
    }
    current_backoff_ms_ = std::min(current_backoff_ms_, kMaxBackoffMs);
    const double jitter = utils::RandRange(1 - kJitter, 1 + kJitter);
    return std::chrono::milliseconds{std::llround(current_backoff_ms_ * jitter)};
}

void RetryBackoff::Reset() {
    current_backoff_ms_ = kInitialBackoffMs;
    initial_ = true;
}

}  // namespace ugrpc::client::impl

USERVER_NAMESPACE_END
