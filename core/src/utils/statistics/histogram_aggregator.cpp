#include <userver/utils/statistics/histogram_aggregator.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/impl/histogram_bucket.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <utils/statistics/impl/histogram_view_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

HistogramAggregator::HistogramAggregator(utils::span<const double> upper_bounds)
    : buckets_(std::make_unique<impl::histogram::Bucket[]>(upper_bounds.size() +
                                                           1)) {
  impl::histogram::CopyBounds(buckets_.get(), upper_bounds);
}

HistogramAggregator::HistogramAggregator(HistogramAggregator&&) noexcept =
    default;

HistogramAggregator& HistogramAggregator::operator=(
    HistogramAggregator&&) noexcept = default;

HistogramAggregator::~HistogramAggregator() = default;

void HistogramAggregator::Add(HistogramView other) {
  UASSERT(buckets_);
  impl::histogram::Add(buckets_.get(), other);
}

void HistogramAggregator::AccountAt(std::size_t bucket_index,
                                    std::uint64_t count) noexcept {
  UASSERT(buckets_);
  UASSERT(bucket_index < GetView().GetBucketCount());
  const auto buckets = impl::histogram::Access::Buckets(
      impl::histogram::MutableView{buckets_.get()});
  impl::histogram::AddNonAtomic(buckets.begin()[bucket_index].counter, count);
}

void HistogramAggregator::AccountInf(std::uint64_t count) noexcept {
  UASSERT(buckets_);
  impl::histogram::AddNonAtomic(buckets_[0].counter, count);
}

void HistogramAggregator::Reset() noexcept {
  UASSERT(buckets_);
  impl::histogram::ResetMetric(buckets_.get());
}

HistogramView HistogramAggregator::GetView() const& noexcept {
  UASSERT(buckets_);
  return impl::histogram::MakeView(buckets_.get());
}

void DumpMetric(Writer& writer, const HistogramAggregator& histogram) {
  writer = histogram.GetView();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
