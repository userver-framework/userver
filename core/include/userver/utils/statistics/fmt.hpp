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
#include <userver/utils/statistics/rate.hpp>

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::LabelView> {
  constexpr static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::utils::statistics::LabelView value,
              FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}={}", value.Name(), value.Value());
  }
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::Label>
    : public fmt::formatter<USERVER_NAMESPACE::utils::statistics::LabelView> {
  template <typename FormatContext>
  auto format(const USERVER_NAMESPACE::utils::statistics::Label& value,
              FormatContext& ctx) const {
    return formatter<USERVER_NAMESPACE::utils::statistics::LabelView>::format(
        USERVER_NAMESPACE::utils::statistics::LabelView{value}, ctx);
  }
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::LabelsSpan> {
  constexpr static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::utils::statistics::LabelsSpan value,
              FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{}", fmt::join(value, ";"));
  }
};

template <>
class fmt::formatter<USERVER_NAMESPACE::utils::statistics::Rate> {
 public:
  constexpr auto parse(format_parse_context& ctx) {
    return rate_format_.parse(ctx);
  }

  template <typename FormatCtx>
  auto format(const USERVER_NAMESPACE::utils::statistics::Rate& rate,
              FormatCtx& ctx) const {
    return rate_format_.format(rate.value, ctx);
  }

 private:
  fmt::formatter<USERVER_NAMESPACE::utils::statistics::Rate::ValueType>
      rate_format_;
};

template <>
struct fmt::formatter<USERVER_NAMESPACE::utils::statistics::HistogramView> {
  // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
  constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatCtx>
  auto format(USERVER_NAMESPACE::utils::statistics::HistogramView histogram,
              FormatCtx& ctx) const {
    const auto bucket_count = histogram.GetBucketCount();
    for (std::size_t i = 0; i < bucket_count; ++i) {
      ctx.advance_to(fmt::format_to(ctx.out(), "[{}]={}",
                                    histogram.GetUpperBoundAt(i),
                                    histogram.GetValueAt(i)));
      ctx.advance_to(fmt::format_to(ctx.out(), ","));
    }
    ctx.advance_to(
        fmt::format_to(ctx.out(), "[inf]={}", histogram.GetValueAtInf()));
    return ctx.out();
  }
};

template <>
class fmt::formatter<USERVER_NAMESPACE::utils::statistics::MetricValue> {
 public:
  constexpr auto parse(format_parse_context& ctx) {
    // To avoid including heavy <algorithm> header.
    const auto max = [](auto a, auto b) { return a > b ? a : b; };
    return max(int_format_.parse(ctx),
               max(float_format_.parse(ctx),
                   max(rate_format_.parse(ctx), histogram_format_.parse(ctx))));
  }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::utils::statistics::MetricValue value,
              FormatContext& ctx) const {
    return value.Visit(USERVER_NAMESPACE::utils::Overloaded{
        [&](std::int64_t x) { return int_format_.format(x, ctx); },
        [&](USERVER_NAMESPACE::utils::statistics::Rate x) {
          return rate_format_.format(x.value, ctx);
        },
        [&](double x) { return float_format_.format(x, ctx); },
        [&](USERVER_NAMESPACE::utils::statistics::HistogramView x) {
          return histogram_format_.format(x, ctx);
        }});
  }

 private:
  // TODO use fmt::formatter<std::variant> when fmt 9 becomes available.
  fmt::formatter<std::int64_t> int_format_;
  fmt::formatter<USERVER_NAMESPACE::utils::statistics::Rate::ValueType>
      rate_format_;
  fmt::formatter<double> float_format_;
  fmt::formatter<USERVER_NAMESPACE::utils::statistics::HistogramView>
      histogram_format_;
};
