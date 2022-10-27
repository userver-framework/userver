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

class JsonFormat final : public utils::statistics::BaseFormatBuilder {
 public:
  explicit JsonFormat() = default;

  void HandleMetric(std::string_view path, utils::statistics::LabelsSpan labels,
                    const MetricValue& value) override {
    formats::json::ValueBuilder node;
    std::visit([&node](const auto& v) { node["value"] = v; }, value);
    node["labels"] = BuildLabels(labels);

    builder_[std::string{path}].PushBack(std::move(node));
  }

  std::string GetString() && { return ToString(builder_.ExtractValue()); }

 private:
  static formats::json::ValueBuilder BuildLabels(
      utils::statistics::LabelsSpan labels) {
    formats::json::ValueBuilder result{formats::common::Type::kObject};

    for (const auto& label : labels) {
      result[std::string{label.Name()}] = label.Value();
    }

    return result;
  }

  formats::json::ValueBuilder builder_{formats::common::Type::kObject};
};

}  // namespace

std::string ToJsonFormat(const utils::statistics::Storage& statistics,
                         const utils::statistics::StatisticsRequest& request) {
  JsonFormat builder{};
  statistics.VisitMetrics(builder, request);
  return std::move(builder).GetString();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
