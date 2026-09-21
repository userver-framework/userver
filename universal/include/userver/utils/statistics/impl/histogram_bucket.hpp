#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {
class Writer;
class HistogramView;
}  // namespace utils::statistics

namespace utils::statistics::impl::histogram {

// The additional first HistogramBucket contains:
// - size in 'upper_bound'
// - inf count in 'counter'
union BoundOrSize {
    double bound;
    std::size_t size;
};

// We make sure that nearby Buckets do not interfere with each other, while `counter` and `sum` reside in the same
// cache line and gain benefits of interference.
struct alignas(concurrent::impl::kDestructiveInterferenceSize) Bucket final {
    constexpr Bucket() noexcept = default;

    Bucket(const Bucket& other) noexcept;
    Bucket& operator=(const Bucket& other) noexcept;

    BoundOrSize upper_bound{0.0};

    // Reside in same cache line, atomic operations gain benefits of constructive interference size.
    std::atomic<std::uint64_t> counter{0};
    std::atomic<double> sum{0.0};
};

void CopyBounds(Bucket* bucket_array, utils::span<const double> upper_bounds);

void CopyBoundsAndValues(Bucket* destination_array, HistogramView source);

void Account(Bucket* bucket_array, double value, std::uint64_t count) noexcept;

void Add(Bucket* bucket_array, HistogramView other);

void ResetMetric(Bucket* bucket_array) noexcept;

HistogramView MakeView(const Bucket* bucket_array) noexcept;

}  // namespace utils::statistics::impl::histogram

USERVER_NAMESPACE_END
