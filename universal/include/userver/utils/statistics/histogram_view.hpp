#pragma once

/// @file userver/utils/statistics/histogram_view.hpp
/// @brief @copybrief utils::statistics::HistogramView

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

class Writer;

namespace impl::histogram {
struct Bucket;
struct Access;
}  // namespace impl::histogram

/// @ingroup userver_universal
///
/// @brief The non-owning reader API for "histogram" metrics.
///
/// @see utils::statistics::Histogram for details on semantics
///
/// HistogramView is cheap to copy, expected to be passed around by value.
class HistogramView final {
public:
    // trivially copyable
    constexpr HistogramView(const HistogramView&) noexcept = default;
    constexpr HistogramView& operator=(const HistogramView&) noexcept = default;

    /// Returns the number of "normal" (non-"infinity") buckets.
    std::size_t GetBucketCount() const noexcept;

    /// Returns the upper bucket boundary for the given bucket.
    double GetUpperBoundAt(std::size_t index) const;

    /// Returns the occurrence count for the given bucket.
    std::uint64_t GetValueAt(std::size_t index) const;

    /// Returns the occurrence count for the "infinity" bucket
    /// (greater than the largest bucket boundary).
    std::uint64_t GetValueAtInf() const noexcept;

    /// Returns the sum of counts from all buckets.
    std::uint64_t GetTotalCount() const noexcept;

    /// Returns sum of values from the given bucket.
    double GetSumAt(std::size_t index) const;

    // Returns sum of values from the "infinity" bucket
    /// (greater than the largest bucket boundary).
    double GetSumAtInf() const noexcept;

    /// Returns sum of values from all buckets.
    double GetTotalSum() const noexcept;

private:
    friend struct impl::histogram::Access;

    constexpr explicit HistogramView(const impl::histogram::Bucket& buckets) noexcept : buckets_(&buckets) {}

    const impl::histogram::Bucket* buckets_;
};

/// Compares equal if bounds are close and values are equal.
bool operator==(HistogramView lhs, HistogramView rhs) noexcept;

/// @overload
bool operator!=(HistogramView lhs, HistogramView rhs) noexcept;

}  // namespace utils::statistics

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::HistogramView> {
    constexpr static auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatCtx>
    auto format(USERVER_NAMESPACE::utils::statistics::HistogramView histogram, FormatCtx& ctx) const {
        const auto bucket_count = histogram.GetBucketCount();
        for (std::size_t i = 0; i < bucket_count; ++i) {
            ctx.advance_to(fmt::format_to(ctx.out(), "[{}]={}", histogram.GetUpperBoundAt(i), histogram.GetValueAt(i)));
            ctx.advance_to(fmt::format_to(ctx.out(), ","));
        }
        ctx.advance_to(fmt::format_to(ctx.out(), "[inf]={}", histogram.GetValueAtInf()));
        return ctx.out();
    }
};
