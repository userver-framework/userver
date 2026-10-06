#include <logging/sampling.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

constexpr std::uint64_t kSamplingRange = std::uint64_t{1} << 32;
constexpr std::uint64_t kSamplingDisabled = std::numeric_limits<std::uint64_t>::max();

constinit std::atomic<std::uint64_t> log_sampling_threshold{kSamplingDisabled};
static_assert(decltype(log_sampling_threshold)::is_always_lock_free);

}  // namespace

void SetLogSamplingProbability(std::optional<double> probability) noexcept {
    auto value = kSamplingDisabled;
    if (probability.has_value() && !std::isnan(probability.value())) {
        // Scale to the full uint32_t RNG range and round down to 2^-32.
        // uint64_t also holds the 2^32 threshold for full sampling;
        // the disabled sentinel stays outside the valid threshold range.
        value = static_cast<std::uint64_t>(std::clamp(probability.value(), 0.0, 1.0) * kSamplingRange);
    }
    log_sampling_threshold.store(value, std::memory_order_relaxed);
}

std::optional<double> GetLogSamplingProbability() noexcept {
    const auto threshold = log_sampling_threshold.load(std::memory_order_relaxed);
    if (threshold == kSamplingDisabled) {
        return std::nullopt;
    }
    return static_cast<double>(threshold) / kSamplingRange;
}

namespace impl {

bool ShouldDropBySampling(Level level) noexcept {
    if (level >= Level::kWarning) {
        return false;
    }
    const auto threshold = log_sampling_threshold.load(std::memory_order_relaxed);
    if (threshold >= kSamplingRange) {
        return false;
    }
    if (threshold == 0) {
        return true;
    }
    try {
        return utils::Rand() >= threshold;
    } catch (const std::exception& e) {
        // on RNG failure prefer writing the record
        UASSERT_MSG(false, e.what());
        return false;
    }
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
