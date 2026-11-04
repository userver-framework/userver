#include <userver/utils/statistics/json.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <userver/formats/common/type.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/statistics/histogram_view.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <utils/statistics/impl/histogram_serialization.hpp>
#include <utils/statistics/impl/rate_serialization.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

constexpr std::size_t kMetricFieldCount = 3;
class JsonFormat final : public utils::statistics::BaseFormatBuilder {
public:
    explicit JsonFormat() = default;

    void HandleMetric(std::string_view path, utils::statistics::LabelsSpan labels, const MetricValue& value) override {
        formats::json::ValueBuilder node{formats::common::Type::kObject};
        node.Reserve(kMetricFieldCount);
        value.Visit([&node](const auto& metric) { node.EmplaceNocheck("value", metric); });
        node.EmplaceNocheck("labels", BuildLabels(labels));
        node.EmplaceNocheck(
            "type",
            value.Visit(utils::Overloaded{
                [](std::int64_t) -> std::string_view { return "GAUGE"; },
                [](double) -> std::string_view { return "GAUGE"; },
                [](Rate) -> std::string_view { return "RATE"; },
                [](HistogramView) -> std::string_view { return "HIST_RATE"; },
            })
        );

        builder_[std::string{path}].PushBack(std::move(node));
    }

    std::string GetString() && { return ToString(builder_.ExtractValue()); }

private:
    static formats::json::ValueBuilder BuildLabels(utils::statistics::LabelsSpan labels) {
        formats::json::ValueBuilder result{formats::common::Type::kObject};
        result.Reserve(labels.size());

        for (const auto& label : labels) {
            result[std::string{label.Name()}] = label.Value();
        }

        return result;
    }

    formats::json::ValueBuilder builder_{formats::common::Type::kObject};
};

}  // namespace
std::string ToJsonFormat(WriterFuncRef writer, const Request& request) {
    JsonFormat builder{};
    VisitMetrics(writer, builder, request);
    return std::move(builder).GetString();
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
