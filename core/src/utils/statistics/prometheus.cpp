#include <userver/utils/statistics/prometheus.hpp>

#include <algorithm>
#include <iterator>
#include <unordered_map>

#include <fmt/format.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace impl {

namespace {

class FormatBuilder final : public utils::statistics::BaseExposeFormatBuilder {
 public:
  explicit FormatBuilder(
      const std::unordered_map<std::string, std::string>& common_labels) {
    fmt::memory_buffer buf;
    bool comma = false;
    for (const auto& [key, value] : common_labels) {
      fmt::format_to(buf, "{}{}=\"", comma ? ',' : '{', key);
      std::replace_copy(value.cbegin(), value.cend(), std::back_inserter(buf),
                        '"', '\'');
      buf.push_back('"');
      comma = true;
    }
    common_labels_ = fmt::to_string(buf);
  }

  void HandleMetric(std::string_view path,
                    const std::vector<utils::statistics::Label>& labels,
                    const MetricValue& value) override {
    DumpMetricName(std::string{path});
    DumpLabels(labels);
    if (const auto* int_value = std::get_if<int64_t>(&value)) {
      fmt::format_to(buf_, " {}", *int_value);
    } else {
      fmt::format_to(buf_, " {}", std::get<double>(value));
    }
    buf_.push_back('\n');
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
    fmt::format_to(buf_, "# TYPE {0} gauge\n{0}", prometheus_name);
  }

  void DumpLabels(const std::vector<utils::statistics::Label>& labels) {
    bool sep = false;
    if (!common_labels_.empty()) {
      buf_.append(common_labels_);
      sep = true;
    } else {
      buf_.push_back('{');
    }
    for (const auto& label : labels) {
      if (sep) {
        buf_.push_back(',');
      }
      fmt::format_to(buf_, "{}=\"", impl::ToPrometheusLabel(label.Name()));
      const auto& value = label.Value();
      std::replace_copy(value.cbegin(), value.cend(), std::back_inserter(buf_),
                        '"', '\'');
      buf_.push_back('"');
      sep = true;
    }
    buf_.push_back('}');
  }

  std::string common_labels_;
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

std::string ToPrometheusFormat(
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Storage& statistics,
    const utils::statistics::StatisticsRequest& statistics_request) {
  impl::FormatBuilder builder{common_labels};
  statistics.VisitMetrics(builder, statistics_request);
  return builder.Release();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
