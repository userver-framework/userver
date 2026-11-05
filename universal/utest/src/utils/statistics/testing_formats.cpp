#include <utils/statistics/testing_formats.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ranges>
#include <string>
#include <vector>

#include <fmt/format.h>

#include <userver/formats/common/items.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/statistics/histogram_aggregator.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {
namespace {

constexpr std::string_view kLabelSeparator = " = ";
constexpr std::string_view kTypeField = "type";
constexpr std::string_view kValueField = "value";
constexpr std::string_view kLabelsField = "labels";
constexpr std::string_view kUseFromJson = "Field 'labels' belongs to the FromJson format. Use Snapshot::FromJson";

template <class T>
void WriteAt(Writer& writer, std::string_view path, const T& value, const std::vector<Label>& labels) {
    const auto views = utils::AsContainer<
        std::vector<LabelView>>(labels | std::views::transform([](const Label& label) { return LabelView{label}; }));
    const auto span = LabelsSpan{views};
    if (path.empty()) {
        writer.ValueWithLabels(value, span);
    } else {
        writer[path].ValueWithLabels(value, span);
    }
}

std::vector<Label> ParseLabels(const formats::json::Value& labels) {
    labels.CheckObject();
    std::vector<Label> result;
    result.reserve(labels.GetSize());
    for (const auto& [name, value] : formats::common::Items(labels)) {
        result.emplace_back(name, value.As<std::string>());
    }
    return result;
}

MetricValue ParseGaugeValue(const formats::json::Value& value) {
    if (value.IsInt64()) {
        return MetricValue{value.As<std::int64_t>()};
    }
    return MetricValue{value.As<double>()};
}

Rate ParseRateValue(const formats::json::Value& value) { return Rate{value.As<std::uint64_t>()}; }

std::vector<double> ParseHistogramBounds(const formats::json::Value& value) {
    const auto bounds = value.As<std::vector<double>>();
    if (!std::ranges::all_of(bounds, [](double bound) { return std::isnormal(bound) && bound > 0; })) {
        throw formats::json::ConversionException("Histogram bounds must be positive normal numbers", value.GetPath());
    }
    if (!std::ranges::is_sorted(bounds)) {
        throw formats::json::ConversionException("Histogram bounds must be sorted", value.GetPath());
    }
    if (std::ranges::adjacent_find(bounds) != bounds.end()) {
        throw formats::json::ConversionException("Histogram bounds must not contain duplicates", value.GetPath());
    }
    return bounds;
}

HistogramAggregator ParseHistogram(const formats::json::Value& value) {
    value.CheckObject();
    const auto bounds = ParseHistogramBounds(value["bounds"]);
    const auto& buckets = value["buckets"];
    buckets.CheckArray();
    if (buckets.GetSize() != bounds.size()) {
        throw formats::json::ConversionException("Histogram buckets size does not match bounds", value.GetPath());
    }

    HistogramAggregator histogram{bounds};
    for (const auto [index, bucket] : utils::enumerate(buckets)) {
        histogram.AccountAt(index, bucket.As<std::uint64_t>());
    }
    histogram.AccountInf(value["inf"].As<std::uint64_t>());
    if (value.HasMember("sum")) {
        histogram.AccountInfSum(utils::impl::InternalTag{}, value["sum"].As<double>());
    }
    return histogram;
}

void WriteTypedValue(
    Writer& writer,
    std::string_view path,
    std::string_view type,
    const formats::json::Value& value,
    const std::vector<Label>& labels
) {
    if (type == "GAUGE") {
        WriteAt(writer, path, ParseGaugeValue(value), labels);
        return;
    }
    if (type == "RATE") {
        WriteAt(writer, path, ParseRateValue(value), labels);
        return;
    }
    if (type == "HIST_RATE") {
        const auto histogram = ParseHistogram(value);
        WriteAt(writer, path, histogram.GetView(), labels);
        return;
    }
    throw formats::json::ConversionException(fmt::format("Unknown metric type '{}'", type), value.GetPath());
}

void WriteFlatMetric(Writer& writer, std::string_view path, const formats::json::Value& metric) {
    metric.CheckObject();
    const auto labels = ParseLabels(metric["labels"]);
    const auto type = metric[kTypeField].As<std::string>();
    WriteTypedValue(writer, path, type, metric[kValueField], labels);
}

Label ParseLabelFromObjectKey(std::string_view key, const formats::json::Value& child) {
    const auto position = key.find(kLabelSeparator);
    if (position == std::string_view::npos) {
        throw formats::json::ConversionException(
            fmt::format("Expected a label key like 'name = value', got '{}'", key),
            child.GetPath()
        );
    }
    const auto name = key.substr(0, position);
    const auto value = key.substr(position + kLabelSeparator.size());
    if (name.empty()) {
        throw formats::json::ConversionException(
            fmt::format("Label name must not be empty, got '{}'", key),
            child.GetPath()
        );
    }
    if (name.ends_with(' ') || value.starts_with(' ')) {
        throw formats::json::ConversionException(
            fmt::format("Expected exactly one space on each side of '=' in a label key, got '{}'", key),
            child.GetPath()
        );
    }
    return Label{std::string{name}, std::string{value}};
}

bool IsHistogramNode(const formats::json::Value& node) {
    return node.IsObject() && node.HasMember("bounds") && node.HasMember("buckets");
}

bool IsTypedLeaf(const formats::json::Value& node) {
    return node.IsObject() && node.HasMember(kTypeField) && node.HasMember(kValueField);
}

void ThrowIfFromJsonLabels(const formats::json::Value& node) {
    if (!node.IsObject() || !node.HasMember(kLabelsField)) {
        return;
    }
    throw formats::json::ConversionException(kUseFromJson, node[kLabelsField].GetPath());
}

void WriteLayeredNode(
    Writer& writer,
    std::string_view path,
    const formats::json::Value& node,
    std::vector<Label>& labels
) {
    if (node.IsArray()) {
        for (const auto& element : node) {
            ThrowIfFromJsonLabels(element);
        }
    }
    ThrowIfFromJsonLabels(node);
    if (IsHistogramNode(node)) {
        const auto histogram = ParseHistogram(node);
        WriteAt(writer, path, histogram.GetView(), labels);
        return;
    }
    if (IsTypedLeaf(node)) {
        WriteTypedValue(writer, path, node[kTypeField].As<std::string>(), node[kValueField], labels);
        return;
    }
    if (node.IsObject()) {
        // A (possibly nested) label level may have zero label key-value alternatives and emit no metrics.
        for (const auto& [key, child] : formats::common::Items(node)) {
            labels.push_back(ParseLabelFromObjectKey(key, child));
            WriteLayeredNode(writer, path, child, labels);
            labels.pop_back();
        }
        return;
    }
    WriteAt(writer, path, ParseGaugeValue(node), labels);
}

}  // namespace

void DumpMetric(Writer& writer, const FlatJson& metrics) {
    const auto& json = metrics.json;
    json.CheckObject();
    for (const auto& [path, series] : formats::common::Items(json)) {
        series.CheckArray();
        for (const auto& metric : series) {
            WriteFlatMetric(writer, path, metric);
        }
    }
}

void DumpMetric(Writer& writer, const LayeredJson& metrics) {
    const auto& json = metrics.json;
    json.CheckObject();
    std::vector<Label> labels;
    for (const auto& [path, node] : formats::common::Items(json)) {
        WriteLayeredNode(writer, path, node, labels);
    }
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
