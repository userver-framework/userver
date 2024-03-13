#include <userver/utils/statistics/prometheus.hpp>

#include <algorithm>
#include <iterator>
#include <unordered_map>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/utils/impl/transparent_hash.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/statistics/fmt.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

namespace {

enum class Typed { kYes, kNo };

template <Typed IsTyped>
class FormatBuilder final : public utils::statistics::BaseFormatBuilder {
 public:
  explicit FormatBuilder() = default;

  void HandleMetric(std::string_view path, utils::statistics::LabelsSpan labels,
                    const MetricValue& value) override {
    if (value.IsHistogram()) {
      HandleHistogram(path, labels, value);
      return;
    }

    DumpMetricNameAndType(path, value);
    DumpLabels(labels);
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE(" {}\n"), value);
  }

  std::string Release() { return fmt::to_string(buf_); }

 private:
  void AppendHistogramMetric(std::string_view metric_suffix,
                             std::string_view path,
                             const std::string_view& upper_bound,
                             std::string_view value,
                             utils::statistics::LabelsSpan labels) {
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("{}_{}{{"), path,
                   metric_suffix);
    if (!upper_bound.empty()) {
      fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("le=\"{}\""),
                     upper_bound);
    }
    if (!labels.empty()) {
      if (!upper_bound.empty()) {
        fmt::format_to(std::back_inserter(buf_), ",");
      }
      DumpLabelsRaw(labels);
    }
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("}} {}\n"), value);
  }

  void HandleHistogram(std::string_view path,
                       utils::statistics::LabelsSpan labels,
                       const MetricValue& value) {
    static constexpr std::string_view kBucket = "bucket";

    const auto prometheus_name = impl::ToPrometheusName(path);
    DumpMetricType(prometheus_name, value);

    auto histogram = value.AsHistogram();
    const auto bucket_count = histogram.GetBucketCount();
    std::uint64_t cumulative_sum = 0;
    for (std::size_t i = 0; i < bucket_count; ++i) {
      cumulative_sum += histogram.GetValueAt(i);
      AppendHistogramMetric(kBucket, prometheus_name,
                            fmt::to_string(histogram.GetUpperBoundAt(i)),
                            fmt::to_string(cumulative_sum), labels);
    }
    cumulative_sum += histogram.GetValueAtInf();
    AppendHistogramMetric(kBucket, prometheus_name, "+Inf",
                          fmt::to_string(cumulative_sum), labels);
    AppendHistogramMetric("count", prometheus_name,
                          /* upper_bound */ "",
                          fmt::to_string(histogram.GetTotalCount()), labels);
  }

  void DumpMetricNameAndType(std::string_view name, const MetricValue& value) {
    if (const auto* const converted =
            utils::impl::FindTransparentOrNullptr(metrics_, name)) {
      buf_.append(*converted);
      return;
    }

    auto prometheus_name = impl::ToPrometheusName(name);
    DumpMetricType(prometheus_name, value);
    buf_.append(prometheus_name);
    metrics_.emplace(name, std::move(prometheus_name));
  }

  void DumpMetricType([[maybe_unused]] std::string_view prometheus_name,
                      [[maybe_unused]] const MetricValue& value) {
    if constexpr (IsTyped == Typed::kNo) {
      const bool should_skip = value.Visit(utils::Overloaded{
          [](std::int64_t) { return true; },
          [](double) { return true; },
          [](Rate) { return false; },
          [](HistogramView) { return false; },
      });
      if (should_skip) return;
    }

    const auto type = value.Visit(utils::Overloaded{
        [](std::int64_t) -> std::string_view { return "gauge"; },
        [](double) -> std::string_view { return "gauge"; },
        [](Rate) -> std::string_view { return "counter"; },
        [](HistogramView) -> std::string_view { return "histogram"; },
    });
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("# TYPE {} {}\n"),
                   prometheus_name, type);
  }

  void DumpLabelsRaw(utils::statistics::LabelsSpan labels) {
    bool sep = false;
    for (const auto& label : labels) {
      if (sep) {
        buf_.push_back(',');
      }
      fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("{}=\""),
                     impl::ToPrometheusLabel(label.Name()));
      const auto& value = label.Value();
      std::replace_copy(value.cbegin(), value.cend(), std::back_inserter(buf_),
                        '"', '\'');
      buf_.push_back('"');
      sep = true;
    }
  }

  void DumpLabels(utils::statistics::LabelsSpan labels) {
    buf_.push_back('{');
    DumpLabelsRaw(labels);
    buf_.push_back('}');
  }

  fmt::memory_buffer buf_;
  utils::impl::TransparentMap<std::string, std::string> metrics_;
};

}  // namespace

std::string ToPrometheusName(std::string_view data) {
  std::string name;
  if (!data.empty()) {
    if (!std::isalpha(data.front())) {
      name = "_";
    }
    name.reserve(name.size() + data.size());
    for (auto c : data) {
      if (std::isalnum(c)) {
        name.push_back(c);
      } else {
        name.push_back('_');
      }
    }
  }
  return name;
}

std::string ToPrometheusLabel(std::string_view name) {
  std::string converted = impl::ToPrometheusName(name);
  auto pos = converted.find_first_not_of('_');
  if (pos == std::string::npos) {
    return {};
  }
  if (pos > 0) {
    --pos;
  }
  return std::move(converted).substr(pos);
}

}  // namespace impl

std::string ToPrometheusFormat(const utils::statistics::Storage& statistics,
                               const utils::statistics::Request& request) {
  impl::FormatBuilder<impl::Typed::kYes> builder{};
  statistics.VisitMetrics(builder, request);
  return builder.Release();
}

std::string ToPrometheusFormatUntyped(
    const utils::statistics::Storage& statistics,
    const utils::statistics::Request& request) {
  impl::FormatBuilder<impl::Typed::kNo> builder{};
  statistics.VisitMetrics(builder, request);
  return builder.Release();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
