#pragma once

#include <memory>

#include <userver/utils/span.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/histogram_view.hpp>

/// @file userver/utils/statistics/histogram_aggregator.hpp
/// @brief @copybrief utils::statistics::HistogramAggregator

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Used to aggregate multiple utils::statistics::Histogram metrics.
///
/// Usage example:
/// @snippet utils/statistics/histogram_test.cpp  HistogramAggregator
class HistogramAggregator final {
 public:
  explicit HistogramAggregator(utils::span<const double> upper_bounds);

  HistogramAggregator(HistogramAggregator&&) noexcept;
  HistogramAggregator& operator=(HistogramAggregator&&) noexcept;
  ~HistogramAggregator();

  /// @brief Add the other histogram to the current one.
  ///
  /// Bucket borders in `this` and `other` must be either identical, or bucket
  /// borders in `this` must be a strict subset of bucket borders in `other`.
  ///
  /// Writes to `*this` are non-atomic.
  void Add(HistogramView other);

  /// Allows reading the histogram.
  HistogramView GetView() const& noexcept;

  /// @cond
  // Store Histogram in a variable before taking a view on it.
  HistogramView GetView() && noexcept = delete;
  /// @endcond

 private:
  std::unique_ptr<impl::histogram::Bucket[]> buckets_;
};

/// Metric serialization support for HistogramAggregator.
void DumpMetric(Writer& writer, const HistogramAggregator& histogram);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
