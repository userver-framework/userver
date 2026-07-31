#pragma once

/// @file userver/utils/aimd_limiter.hpp
/// @brief @copybrief utils::AimdLimiter

#include <atomic>
#include <cstddef>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_universal userver_concurrency
///
/// @brief Thread safe AIMD (Additive Increase Multiplicative Decrease) limiter
///
/// Keeps an adaptive limit within `[min_limit, max_limit]`:
///
/// * on success the limit is additively increased:
///   `limit = min(max_limit, limit + alpha / limit)`;
/// * on failure the limit is multiplicatively decreased:
///   `limit = max(min_limit, limit * beta)`.
///
/// If `max_limit` is set below `Policy::min_limit`, `max_limit` wins and
/// GetCurrentLimit() returns `max_limit`.
class AimdLimiter final {
public:
    /// AIMD limit update policy
    struct Policy {
        /// Lower bound for the limit, must be positive
        std::size_t min_limit{2};
        /// Additive increase coefficient, must be positive and finite
        double alpha{1.0};
        /// Multiplicative decrease coefficient, must be in range (0, 1)
        double beta{0.5};
    };

    /// Create a limiter with the current limit set to `max_limit`
    /// @throws std::runtime_error if the policy is invalid
    AimdLimiter(std::size_t max_limit, Policy policy);

    AimdLimiter(const AimdLimiter&) = delete;
    AimdLimiter(AimdLimiter&&) = delete;
    AimdLimiter& operator=(const AimdLimiter&) = delete;
    AimdLimiter& operator=(AimdLimiter&&) = delete;

    /// Set the upper bound for the limit, clamping down the current limit if needed
    void SetMaxLimit(std::size_t max_limit) noexcept;

    /// Set limit update policy
    /// @throws std::runtime_error if the policy is invalid
    void SetPolicy(Policy policy);

    /// Get the upper bound for the limit (might be inaccurate as the result is stale)
    std::size_t GetMaxLimit() const noexcept;

    /// Get current limit (might be inaccurate as the result is stale)
    std::size_t GetCurrentLimit() const noexcept;

    /// Additively increase the limit up to the max limit
    void OnSuccess() noexcept;

    /// Multiplicatively decrease the limit down to the min limit
    void OnFailure() noexcept;

private:
    std::atomic<std::size_t> max_limit_;
    std::atomic<std::size_t> min_limit_;
    std::atomic<double> alpha_;
    std::atomic<double> beta_;
    std::atomic<double> current_limit_;
};

}  // namespace utils

USERVER_NAMESPACE_END
