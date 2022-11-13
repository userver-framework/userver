#include <userver/utils/statistics/prometheus.hpp>

#include <algorithm>
#include <iterator>
#include <unordered_map>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <userver/utils/algo.hpp>
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
    DumpMetricName(std::string{path});
    DumpLabels(labels);
    fmt::format_to(std::back_inserter(buf_), FMT_COMPILE(" {}\n"), value);
  }

  std::string Release() { return fmt::to_string(buf_); }

 private:
  void DumpMetricName(const std::string& name) {
    if (auto* converted = utils::FindOrNullptr(metrics_, name)) {
      buf_.append(*converted);
      return;
    }

    auto prometheus_name = impl::ToPrometheusName(name);
    metrics_.emplace(name, prometheus_name);
    if constexpr (IsTyped == Typed::kYes) {
      fmt::format_to(std::back_inserter(buf_), FMT_COMPILE("# TYPE {} gauge\n"),
                     prometheus_name);
    }
    buf_.append(prometheus_name);
  }

  void DumpLabels(utils::statistics::LabelsSpan labels) {
    buf_.push_back('{');
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
    buf_.push_back('}');
  }

  fmt::memory_buffer buf_;
  std::unordered_map<std::string, std::string> metrics_;
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
  return converted.substr(pos);
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
