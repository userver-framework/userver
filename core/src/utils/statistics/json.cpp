#include <utils/statistics/visitation.hpp>

#include <algorithm>
#include <iterator>
#include <optional>
#include <stack>
#include <string_view>
#include <userver/formats/json.hpp>
#include <variant>
#include <vector>

#include <fmt/format.h>

#include <userver/formats/common/items.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/from_string.hpp>
#include <userver/utils/statistics/storage.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

class JsonFormat final : public utils::statistics::BaseExposeFormatBuilder {
 public:
  explicit JsonFormat(
      const std::unordered_map<std::string, std::string>& common_labels)
      : common_labels_{common_labels} {}

  void HandleMetric(std::string_view path,
                    const std::vector<utils::statistics::Label>& labels,
                    const MetricValue& value) override {
    formats::json::ValueBuilder node;
    std::visit([&node](const auto& v) { node["value"] = v; }, value);
    node["labels"] = BuildLabels(labels);

    builder_[std::string{path}].PushBack(std::move(node));
  }

  std::string GetString() && { return ToString(builder_.ExtractValue()); }

 private:
  formats::json::ValueBuilder BuildLabels(
      const std::vector<utils::statistics::Label>& labels) {
    formats::json::ValueBuilder result{formats::common::Type::kObject};

    for (const auto& label : labels) {
      result[label.Name()] = label.Value();
    }

    for (const auto& [name, value] : common_labels_) {
      result[name] = value;
    }

    return result;
  }

  const std::unordered_map<std::string, std::string>& common_labels_;
  formats::json::ValueBuilder builder_{formats::common::Type::kObject};
};

}  // namespace

std::string ToJsonFormat(
    const std::unordered_map<std::string, std::string>& common_labels,
    const utils::statistics::Storage& statistics,
    const utils::statistics::StatisticsRequest& statistics_request) {
  JsonFormat builder{common_labels};
  statistics.VisitMetrics(builder, statistics_request);
  return std::move(builder).GetString();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
