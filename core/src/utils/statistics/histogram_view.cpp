#include <userver/utils/statistics/histogram_view.hpp>

#include <type_traits>

#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/impl/histogram_bucket.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <utils/statistics/impl/histogram_view_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

static_assert(std::is_trivially_copyable_v<HistogramView> &&
                  sizeof(HistogramView) <= sizeof(void*) * 2,
              "HistogramView should fit in registers, because it is expected "
              "to be passed around by value");

std::size_t HistogramView::GetBucketCount() const noexcept {
  UASSERT(buckets_);
  return buckets_[0].upper_bound.size;
}

double HistogramView::GetUpperBoundAt(std::size_t index) const {
  UASSERT(index < GetBucketCount());
  const auto result = buckets_[index + 1].upper_bound.bound;
  UASSERT(result > 0);
  return result;
}

std::uint64_t HistogramView::GetValueAt(std::size_t index) const {
  UASSERT(index < GetBucketCount());
  return buckets_[index + 1].counter.load(std::memory_order_relaxed);
}

std::uint64_t HistogramView::GetValueAtInf() const noexcept {
  UASSERT(buckets_);
  return buckets_[0].counter.load(std::memory_order_relaxed);
}

std::uint64_t HistogramView::GetTotalCount() const noexcept {
  const auto bucket_count = GetBucketCount();
  auto total = GetValueAtInf();
  for (std::size_t i = 0; i < bucket_count; ++i) {
    total += GetValueAt(i);
  }
  return total;
}

double HistogramView::GetSumAt(std::size_t index) const {
  UASSERT(index < GetBucketCount());
  return buckets_[index + 1].sum.load(std::memory_order_relaxed);
}

double HistogramView::GetSumAtInf() const noexcept {
  UASSERT(buckets_);
  return buckets_[0].sum.load(std::memory_order_relaxed);
}

double HistogramView::GetTotalSum() const noexcept {
  const auto bucket_count = GetBucketCount();
  auto total = GetSumAtInf();
  for (std::size_t i = 0; i < bucket_count; ++i) {
    total += GetSumAt(i);
  }
  return total;
}

void DumpMetric(Writer& writer, HistogramView histogram) { writer = histogram; }

bool operator==(HistogramView lhs, HistogramView rhs) noexcept {
  return impl::histogram::HasSameBoundsAndValues(lhs, rhs);
}

bool operator!=(HistogramView lhs, HistogramView rhs) noexcept {
  return !(lhs == rhs);
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
