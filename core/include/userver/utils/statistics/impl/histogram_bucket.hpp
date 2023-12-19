#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

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

struct Bucket final {
  constexpr Bucket() noexcept = default;

  Bucket(const Bucket& other) noexcept;
  Bucket& operator=(const Bucket& other) noexcept;

  BoundOrSize upper_bound{0.0};
  std::atomic<std::uint64_t> counter{0};
};

void CopyBounds(Bucket* bucket_array, utils::span<const double> upper_bounds);

void CopyBoundsAndValues(Bucket* destination_array, HistogramView source);

void Account(Bucket* bucket_array, double value, std::uint64_t count) noexcept;

void Add(Bucket* bucket_array, HistogramView other);

void ResetMetric(Bucket* bucket_array) noexcept;

HistogramView MakeView(const Bucket* bucket_array) noexcept;

}  // namespace utils::statistics::impl::histogram

USERVER_NAMESPACE_END
