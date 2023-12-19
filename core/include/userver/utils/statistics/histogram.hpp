#pragma once

/// @file userver/utils/statistics/histogram.hpp
/// @brief @copybrief utils::statistics::Histogram

#include <cstdint>
#include <memory>

#include <userver/utils/span.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/histogram_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl::histogram {
struct BoundsBlock;
}  // namespace impl::histogram

/// @brief A histogram with a dynamically-allocated array of buckets.
///
/// ## Histogram metrics
///
/// * A histogram metric is a list of counters ("buckets") with specified bounds
/// * The implicit lowest bound is always 0
/// * Bucket bounds must follow the ascending order
/// * Values on the bucket borders fall into the lower bucket
/// * Besides normal buckets, there is also a special "infinity" bucket,
///   which contains values that are greater than the greatest bucket bound
/// * Each individual counter has the semantics of utils::statistics::Rate
/// * For best portability, there should be no more than 50 buckets
///
/// ## Histograms vs utils::statistics::Percentile
///
/// @see utils::statistics::Percentile is a related metric type
///
/// The trade-offs of histograms with `Percentile` are:
///
/// 1. `Percentile` metrics are fundamentally non-summable across multiple
///    hosts. `Histogram`, on the other hand, are summable
/// 2. A `Histogram` takes up more storage space on the statistics server,
///    as there are typically 20-50 buckets in a `Histogram`, but only a few
///    required percentiles in a `Percentile`
/// 3. `Percentile` metrics have almost infinite precision, limited only
///    by the number of allocated atomic counters. The precision of `Histogram`
///    metrics is limited by the initially set bounds
///
/// ## Usage of Histogram
///
/// Usage example:
/// @snippet utils/statistics/histogram_test.cpp  sample
///
/// Contents of a Histogram are read using utils::statistics::HistogramView.
/// This can be useful for writing custom metric serialization formats or
/// for testing.
///
/// Histogram metrics can be summed using
/// utils::statistics::HistogramAggregator.
///
/// Histogram can be used in utils::statistics::MetricTag:
/// @snippet utils/statistics/histogram_test.cpp  metric tag
class Histogram final {
 public:
  /// Sets upper bounds for each non-"infinite" bucket. The lowest bound is
  /// always 0.
  explicit Histogram(utils::span<const double> upper_bounds);

  /// Copies an existing histogram.
  explicit Histogram(HistogramView other);

  Histogram(Histogram&&) noexcept;
  Histogram(const Histogram&);
  Histogram& operator=(Histogram&&) noexcept;
  Histogram& operator=(const Histogram&);
  ~Histogram();

  /// Atomically increment the bucket corresponding to the given value.
  void Account(double value, std::uint64_t count = 1) noexcept;

  /// Atomically reset all counters to zero.
  friend void ResetMetric(Histogram& histogram) noexcept;

  /// Allows reading the histogram.
  HistogramView GetView() const& noexcept;

  /// @cond
  // Store Histogram in a variable before taking a view on it.
  HistogramView GetView() && noexcept = delete;
  /// @endcond

 private:
  void UpdateBounds();

  std::unique_ptr<impl::histogram::Bucket[]> buckets_;
  // B+ tree of bucket bounds for optimization of Account.
  std::unique_ptr<impl::histogram::BoundsBlock[]> bounds_;
  // Duplicate size here to avoid loading an extra cache line in Account.
  std::size_t bucket_count_;
};

/// Metric serialization support for Histogram.
void DumpMetric(Writer& writer, const Histogram& histogram);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
