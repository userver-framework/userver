#include <userver/utils/statistics/impl/histogram_bucket.hpp>

#include <utils/statistics/impl/histogram_view_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl::histogram {

Bucket::Bucket(const Bucket& other) noexcept
    : upper_bound(other.upper_bound),
      counter(other.counter.load(std::memory_order_relaxed)) {}

Bucket& Bucket::operator=(const Bucket& other) noexcept {
  if (this == &other) return *this;
  upper_bound = other.upper_bound;
  counter.store(other.counter.load(std::memory_order_relaxed),
                std::memory_order_relaxed);
  return *this;
}

void CopyBounds(Bucket* bucket_array, utils::span<const double> upper_bounds) {
  MutableView{bucket_array}.SetBounds(upper_bounds);
}

void CopyBoundsAndValues(Bucket* destination_array, HistogramView source) {
  MutableView{destination_array}.Assign(source);
}

void Account(Bucket* bucket_array, double value, std::uint64_t count) noexcept {
  MutableView{bucket_array}.Account(value, count);
}

void Add(Bucket* bucket_array, HistogramView other) {
  MutableView{bucket_array}.Add(other);
}

void ResetMetric(Bucket* bucket_array) noexcept {
  MutableView{bucket_array}.Reset();
}

HistogramView MakeView(const Bucket* bucket_array) noexcept {
  return Access::MakeView(bucket_array);
}

}  // namespace utils::statistics::impl::histogram

USERVER_NAMESPACE_END
