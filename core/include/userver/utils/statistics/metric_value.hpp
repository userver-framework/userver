#pragma once

/// @file userver/utils/statistics/metric_value.hpp
/// @brief @copybrief utils::statistics::MetricValue

#include <cstdint>
#include <variant>

#include <userver/utils/statistics/rate.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

/// @brief The value of a metric. Only integer and floating-point metrics are
/// allowed.
class MetricValue final {
 public:
  using RawType = std::variant<std::int64_t, double, Rate>;

  MetricValue(const MetricValue&) = default;
  MetricValue& operator=(const MetricValue&) = default;

  bool operator==(const MetricValue& other) const noexcept {
    return value_ == other.value_;
  }

  bool operator!=(const MetricValue& other) const noexcept {
    return value_ != other.value_;
  }

  /// @brief Retrieve the value of an integer metric.
  /// @throws std::exception on type mismatch.
  std::int64_t AsInt() const { return std::get<std::int64_t>(value_); }

  /// @brief Retrieve the value of a floating-point metric.
  /// @throws std::exception on type mismatch.
  double AsFloat() const { return std::get<double>(value_); }

  /// @brief Retrieve the value of a Rate metric.
  /// @throws std::exception on type mismatch.
  Rate AsRate() const { return std::get<Rate>(value_); }

  /// @brief Returns whether metric is Rate metric
  bool IsRate() const noexcept { return std::holds_alternative<Rate>(value_); }

  /// @brief Calls @p visitor with either a `std::int64_t` or a `double` value.
  /// @returns Whatever @p visitor returns.
  template <typename VisitorFunc>
  decltype(auto) Visit(VisitorFunc visitor) const {
    return std::visit(visitor, value_);
  }

  /// @cond
  MetricValue() noexcept : value_(std::int64_t{0}) {}

  explicit MetricValue(RawType value) noexcept : value_(value) {}
  /// @endcond

 private:
  RawType value_;
};

}  // namespace utils::statistics

USERVER_NAMESPACE_END
