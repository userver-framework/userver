#include <userver/utils/statistics/json.hpp>

#include <string_view>

#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/overloaded.hpp>
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
    value.Visit([&node](const auto& v) { node["value"] = v; });
    node["labels"] = BuildLabels(labels);
    node["type"] = value.Visit(utils::Overloaded{
        [](const Rate&) -> std::string_view { return "RATE"; },
        [](const auto&) -> std::string_view { return "GAUGE"; }});

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
                         const utils::statistics::Request& request) {
  JsonFormat builder{};
  statistics.VisitMetrics(builder, request);
  return std::move(builder).GetString();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
