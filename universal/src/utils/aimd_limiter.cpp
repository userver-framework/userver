#include <userver/utils/aimd_limiter.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

void ValidatePolicy(const AimdLimiter::Policy& policy) {
    if (policy.min_limit == 0) {
        throw std::runtime_error("AimdLimiter min_limit must be positive");
    }
    if (!(policy.alpha > 0.0) || !std::isfinite(policy.alpha)) {
        throw std::runtime_error("AimdLimiter alpha must be positive and finite");
    }
    if (!(policy.beta > 0.0) || !(policy.beta < 1.0)) {
        throw std::runtime_error("AimdLimiter beta must be in range (0, 1)");
    }
}

}  // namespace

AimdLimiter::AimdLimiter(std::size_t max_limit, Policy policy)
    : max_limit_(max_limit),
      min_limit_(policy.min_limit),
      alpha_(policy.alpha),
      beta_(policy.beta),
      current_limit_(static_cast<double>(max_limit))
{
    ValidatePolicy(policy);
    static_assert(decltype(AimdLimiter::max_limit_)::is_always_lock_free);
    static_assert(decltype(AimdLimiter::min_limit_)::is_always_lock_free);
    static_assert(decltype(AimdLimiter::alpha_)::is_always_lock_free);
    static_assert(decltype(AimdLimiter::beta_)::is_always_lock_free);
    static_assert(decltype(AimdLimiter::current_limit_)::is_always_lock_free);
}

void AimdLimiter::SetMaxLimit(std::size_t max_limit) noexcept {
    max_limit_.store(max_limit);

    const auto max_limit_double = static_cast<double>(max_limit);
    auto expected = current_limit_.load();
    while (expected > max_limit_double) {
        if (current_limit_.compare_exchange_weak(expected, max_limit_double)) {
            break;
        }
    }
}

void AimdLimiter::SetPolicy(Policy policy) {
    ValidatePolicy(policy);
    min_limit_.store(policy.min_limit);
    alpha_.store(policy.alpha);
    beta_.store(policy.beta);
}

std::size_t AimdLimiter::GetMaxLimit() const noexcept { return max_limit_.load(std::memory_order_relaxed); }

std::size_t AimdLimiter::GetCurrentLimit() const noexcept {
    const auto max_limit = static_cast<double>(max_limit_.load());
    // min_limit may exceed max_limit after SetMaxLimit, max_limit wins
    const auto min_limit = std::min(static_cast<double>(min_limit_.load()), max_limit);
    const auto clamped = std::clamp(current_limit_.load(), min_limit, max_limit);
    return static_cast<std::size_t>(clamped);
}

void AimdLimiter::OnSuccess() noexcept {
    const auto max_limit = static_cast<double>(max_limit_.load());
    const auto min_limit = static_cast<double>(min_limit_.load());
    const auto alpha = alpha_.load();

    auto expected = current_limit_.load();
    double updated{};
    do {
        // Guard against division by zero: min_limit >= 1 by Policy validation
        const auto limit = std::max(expected, min_limit);
        updated = std::min(max_limit, limit + alpha / limit);
    } while (!current_limit_.compare_exchange_weak(expected, updated));
}

void AimdLimiter::OnFailure() noexcept {
    const auto min_limit = static_cast<double>(min_limit_.load());
    const auto beta = beta_.load();

    auto expected = current_limit_.load();
    double updated{};
    do {
        updated = std::max(min_limit, expected * beta);
    } while (!current_limit_.compare_exchange_weak(expected, updated));
}

}  // namespace utils

USERVER_NAMESPACE_END
