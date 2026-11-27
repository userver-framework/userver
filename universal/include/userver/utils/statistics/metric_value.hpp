#pragma once

/// @file userver/utils/statistics/metric_value.hpp
/// @brief @copybrief utils::statistics::MetricValue

#include <cstdint>
#include <type_traits>
#include <variant>

#include <fmt/format.h>

#include <userver/utils/statistics/histogram_view.hpp>
#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @ingroup userver_universal
///
/// @brief The value of a metric.
///
/// Cheap to copy, expected to be passed around by value.
///
/// A histogram is stored only as a @ref utils::statistics::HistogramView.
/// Its backing storage has to be kept alive separately.
class MetricValue final {
public:
    using RawType = std::variant<std::int64_t, double, Rate, HistogramView>;

    /// Creates an unspecified metric value.
    constexpr MetricValue() noexcept : value_(std::int64_t{0}) {}

    /// Constructs @ref utils::statistics::MetricValue for tests.
    /// @{
    constexpr /*implicit*/ MetricValue(std::int64_t value) noexcept : value_(value) {}

    constexpr /*implicit*/ MetricValue(double value) noexcept : value_(value) {}

    constexpr /*implicit*/ MetricValue(Rate value) noexcept : value_(value) {}

    constexpr /*implicit*/ MetricValue(HistogramView value) noexcept : value_(value) {}
    /// @}

    // trivially copyable
    MetricValue(const MetricValue&) = default;
    MetricValue& operator=(const MetricValue&) = default;

    friend bool operator==(const MetricValue& lhs, const MetricValue& rhs) noexcept { return lhs.value_ == rhs.value_; }

    friend bool operator!=(const MetricValue& lhs, const MetricValue& rhs) noexcept { return lhs.value_ != rhs.value_; }

    /// @brief Numeric comparison of two metric values.
    ///
    /// `std::int64_t`, `double` and @ref utils::statistics::Rate compare as numbers: `4`, `4.0`
    /// and `Rate{4}` are equal. A fractional `double` is not equal to a
    /// truncated integer. An integer that is not exactly representable as
    /// `double` is not equal to the rounded value.
    ///
    /// @ref utils::statistics::HistogramView compares as a histogram, via
    /// @ref utils::statistics::operator==(HistogramView,HistogramView).
    /// Bucket sums are ignored. A histogram is not equal to a number.
    bool EqualsNumerically(const MetricValue& other) const noexcept;

    /// @brief Retrieve the value of an integer metric.
    /// @throws std::exception on type mismatch.
    std::int64_t AsInt() const { return std::get<std::int64_t>(value_); }

    /// @brief Retrieve the value of a floating-point metric.
    /// @throws std::exception on type mismatch.
    double AsFloat() const { return std::get<double>(value_); }

    /// @brief Retrieve the value of a @ref utils::statistics::Rate metric.
    /// @throws std::exception on type mismatch.
    Rate AsRate() const { return std::get<Rate>(value_); }

    /// @brief Returns whether metric is a @ref utils::statistics::Rate metric.
    bool IsRate() const noexcept { return std::holds_alternative<Rate>(value_); }

    /// @brief Retrieve the value of a @ref utils::statistics::HistogramView metric.
    /// @throws std::exception on type mismatch.
    HistogramView AsHistogram() const { return std::get<HistogramView>(value_); }

    /// @brief Returns whether metric is a @ref utils::statistics::HistogramView metric.
    bool IsHistogram() const noexcept { return std::holds_alternative<HistogramView>(value_); }

    /// @brief Calls @p visitor with the stored value.
    ///
    /// The argument has one of these types:
    /// - `std::int64_t`
    /// - `double`
    /// - @ref utils::statistics::Rate
    /// - @ref utils::statistics::HistogramView
    ///
    /// @returns Whatever @p visitor returns.
    template <typename VisitorFunc>
    decltype(auto) Visit(VisitorFunc visitor) const {
        return std::visit(visitor, value_);
    }

    /// @cond
    explicit MetricValue(RawType value) noexcept : value_(value) {}
    /// @endcond

private:
    RawType value_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END

template <>
class fmt::formatter<USERVER_NAMESPACE::utils::statistics::MetricValue> {
public:
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(USERVER_NAMESPACE::utils::statistics::MetricValue value, FormatContext& ctx) const {
        return value.Visit([&ctx](auto x) { return fmt::format_to(ctx.out(), "{}", x); });
    }
};
