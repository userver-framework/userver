#pragma once

/// @file userver/utils/statistics/fmt.hpp
/// @brief fmt formatters for various statistics types
///
/// - utils::statistics::LabelView
/// - utils::statistics::Label
/// - utils::statistics::LabelsSpan
/// - utils::statistics::MetricValue

#include <variant>

#include <fmt/format.h>

#include <userver/utils/fmt_compat.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/metric_value.hpp>

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::LabelView> {
  constexpr static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::utils::statistics::LabelView value,
              FormatContext& ctx) USERVER_FMT_CONST {
    return fmt::format_to(ctx.out(), "{}={}", value.Name(), value.Value());
  }
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::Label>
    : public fmt::formatter<USERVER_NAMESPACE::utils::statistics::LabelView> {
  template <typename FormatContext>
  auto format(const USERVER_NAMESPACE::utils::statistics::Label& value,
              FormatContext& ctx) USERVER_FMT_CONST {
    return formatter<USERVER_NAMESPACE::utils::statistics::LabelView>::format(
        USERVER_NAMESPACE::utils::statistics::LabelView{value}, ctx);
  }
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::LabelsSpan> {
  constexpr static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::utils::statistics::LabelsSpan value,
              FormatContext& ctx) USERVER_FMT_CONST {
    return fmt::format_to(ctx.out(), "{}", fmt::join(value, ";"));
  }
};

template <>
class fmt::formatter<USERVER_NAMESPACE::utils::statistics::MetricValue> {
 public:
  constexpr auto parse(format_parse_context& ctx) {
    // To avoid including heavy <algorithm> header.
    const auto max = [](auto a, auto b) { return a > b ? a : b; };
    return max(int_format_.parse(ctx), float_format_.parse(ctx));
  }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::utils::statistics::MetricValue value,
              FormatContext& ctx) USERVER_FMT_CONST {
    return value.Visit(USERVER_NAMESPACE::utils::Overloaded{
        [&](std::int64_t x) { return int_format_.format(x, ctx); },
        [&](double x) { return float_format_.format(x, ctx); }});
  }

 private:
  fmt::formatter<std::int64_t> int_format_;
  fmt::formatter<double> float_format_;
};
