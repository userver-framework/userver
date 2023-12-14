#include <userver/utils/statistics/histogram_aggregator.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/statistics/impl/histogram_bucket.hpp>
#include <userver/utils/statistics/writer.hpp>

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

HistogramView HistogramAggregator::GetView() const& noexcept {
  UASSERT(buckets_);
  return impl::histogram::MakeView(buckets_.get());
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
