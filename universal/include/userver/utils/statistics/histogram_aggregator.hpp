#pragma once

/// @file userver/utils/statistics/histogram_aggregator.hpp
/// @brief @copybrief utils::statistics::HistogramAggregator

#include <memory>

#include <userver/utils/impl/internal_tag.hpp>
#include <userver/utils/span.hpp>
#include <userver/utils/statistics/fwd.hpp>
#include <userver/utils/statistics/histogram_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief Used to aggregate multiple @ref utils::statistics::Histogram metrics.
///
/// Usage example:
/// @snippet core/src/utils/statistics/histogram_test.cpp  HistogramAggregator
class HistogramAggregator final {
public:
    /// @brief Sets upper bounds for each non-"infinite" bucket. The lowest bound is
    /// always 0.
    ///
    /// @param upper_bounds is copied inside and is not required to be kept alive after the constructor completes.
    explicit HistogramAggregator(utils::span<const double> upper_bounds);

    /// @brief Empty tag. Selects the constructor that copies bucket borders from a @ref
    /// utils::statistics::HistogramView.
    struct CopyBounds {};

    /// @brief Copies bucket borders from @a view. Bucket counts stay zero.
    ///
    /// Use @ref utils::statistics::HistogramAggregator::Add to copy or merge values afterwards.
    HistogramAggregator(CopyBounds, HistogramView view);

    /// @brief Transfers ownership of the bucket buffer.
    ///
    /// @ref utils::statistics::HistogramView instances already taken from @a other remain valid and refer to that
    /// buffer, now owned by this object.
    HistogramAggregator(HistogramAggregator&&) noexcept;

    /// Views already taken from the left-hand side become invalid: its previous buffer is destroyed.
    /// @see utils::statistics::HistogramAggregator::HistogramAggregator(HistogramAggregator&&)
    HistogramAggregator& operator=(HistogramAggregator&&) noexcept;
    ~HistogramAggregator();

    /// @brief Add the other histogram to the current one.
    ///
    /// Bucket borders in `this` and `other` must be either identical, or bucket
    /// borders in `this` must be a strict subset of bucket borders in `other`.
    ///
    /// Writes to `*this` are non-atomic.
    void Add(HistogramView other);

    /// Non-atomically increment the bucket corresponding to the given index.
    void AccountAt(std::size_t bucket_index, std::uint64_t count = 1) noexcept;

    /// Non-atomically increment the "infinity" bucket.
    void AccountInf(std::uint64_t count = 1) noexcept;

    /// @cond
    /// Add @a sum to the "infinity" bucket sum. The "infinity" count stays unchanged.
    void AccountInfSum(utils::impl::InternalTag, double sum) noexcept;
    /// @endcond

    /// Reset all buckets to zero.
    void Reset() noexcept;

    /// Allows reading the histogram.
    HistogramView GetView() const& noexcept;

    /// @cond
    // Store HistogramAggregator in a variable before taking a view on it.
    HistogramView GetView() && noexcept = delete;
    /// @endcond

private:
    std::unique_ptr<impl::histogram::Bucket[]> buckets_;
};

/// Metric serialization support for @ref utils::statistics::HistogramAggregator.
void DumpMetric(Writer& writer, const HistogramAggregator& histogram);

}  // namespace utils::statistics

USERVER_NAMESPACE_END
