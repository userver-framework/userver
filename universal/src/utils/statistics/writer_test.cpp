#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

#include <userver/utils/statistics/labels.hpp>
#include <userver/utils/statistics/metric_value.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/statistics/rate_counter.hpp>
#include <userver/utils/statistics/request.hpp>
#include <userver/utils/statistics/writer.hpp>

using USERVER_NAMESPACE::utils::statistics::ToPrometheusFormatUntyped;
using USERVER_NAMESPACE::utils::statistics::Writer;

USERVER_NAMESPACE_BEGIN

namespace {

/// [VisitMetrics custom format]
class MetricsMessageBuilder final : public utils::statistics::BaseFormatBuilder {
public:
    void HandleMetric(
        std::string_view path,
        utils::statistics::LabelsSpan labels,
        const utils::statistics::MetricValue& value
    ) override {
        message_ += fmt::format("{}\t{}\t{}\n", path, labels, value);
    }

    const std::string& GetMessage() const { return message_; }

private:
    std::string message_;
};
/// [VisitMetrics custom format]

/// [DumpMetric basic]
struct ComponentMetricsNested {
    std::atomic<int> nested1{17};
    std::atomic<int> nested2{18};
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const ComponentMetricsNested& m) {
    writer["nested1"] = m.nested1;
    writer["nested2"].ValueWithLabels(m.nested2, {"label_name", "label_value"});
}
/// [DumpMetric basic]

/// [DumpMetric nested]
struct ComponentMetrics {
    std::atomic<int> total{42};
    ComponentMetricsNested summary;
    std::unordered_map<std::string, std::shared_ptr<ComponentMetricsNested>> metrics;
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const ComponentMetrics& m) {
    writer["total"] = m.total;
    writer["summary"] = m.summary;
    for (const auto& [key, value] : m.metrics) {
        if (!value) {
            continue;
        }
        writer.ValueWithLabels(*value, {{"a_label", key}, {"b_label", "b"}});
    }
}
/// [DumpMetric nested]

struct NotDumpable {};

namespace some {

struct Dumpable1 {};
[[maybe_unused]] void DumpMetric(utils::statistics::Writer&, const Dumpable1&) {}

struct Dumpable2 {};
[[maybe_unused]] void DumpMetric(utils::statistics::Writer&, Dumpable2) {}

struct Dumpable3 {};

}  // namespace some

}  // namespace

namespace utils::statistics {
[[maybe_unused]] static void DumpMetric(Writer&, some::Dumpable3) {}
}  // namespace utils::statistics

TEST(MetricsWriter, DumpMetricSamples) {
    ComponentMetrics component_metrics;
    component_metrics.metrics.emplace("metric-in-map", std::make_shared<ComponentMetricsNested>());

    const auto formatted = ToPrometheusFormatUntyped([&](Writer& writer) {
        writer = ComponentMetricsNested{};
        writer["more_metrics"] = component_metrics;
    });

    const auto* const expected =
        "nested1{} 17\n"
        "nested2{label_name=\"label_value\"} 18\n"
        "more_metrics_total{} 42\n"
        "more_metrics_summary_nested1{} 17\n"
        "more_metrics_summary_nested2{label_name=\"label_value\"} 18\n"
        "more_metrics_nested1{a_label=\"metric-in-map\",b_label=\"b\"} 17\n"
        "more_metrics_nested2{a_label=\"metric-in-map\",b_label=\"b\",label_name=\"label_value\"} 18\n";

    EXPECT_EQ(expected, formatted);
}
TEST(MetricsWriter, ComplexPathsStored) {
    const auto* const expected2 = "a_b_c_d_e{} 42\n";
    EXPECT_EQ(expected2, ToPrometheusFormatUntyped([](Writer& writer) {
                  auto new_writer = writer["a"]["b"]["c"]["d"];
                  new_writer["e"] = 42;
              }));
}

TEST(MetricsWriter, CustomizationPointChecks) {
    EXPECT_TRUE(utils::statistics::HasWriterSupport<std::atomic<int>>);
    EXPECT_TRUE(utils::statistics::HasWriterSupport<int>);
    EXPECT_TRUE(utils::statistics::HasWriterSupport<long double>);

    EXPECT_FALSE(utils::statistics::HasWriterSupport<NotDumpable>);
    EXPECT_FALSE(utils::statistics::HasWriterSupport<std::string>);
    EXPECT_FALSE(utils::statistics::HasWriterSupport<std::string_view>);

    EXPECT_TRUE(utils::statistics::HasWriterSupport<some::Dumpable1>);
    EXPECT_TRUE(utils::statistics::HasWriterSupport<some::Dumpable2>);
    EXPECT_TRUE(utils::statistics::HasWriterSupport<some::Dumpable3>);
}

TEST(MetricsWriter, VisitMetricsCustomFormat) {
    /// [VisitMetrics custom format usage]
    utils::statistics::RateCounter requests{10};
    utils::statistics::RateCounter errors{2};
    MetricsMessageBuilder builder;

    utils::statistics::VisitMetrics(
        [&](utils::statistics::Writer& writer) {
            writer["requests"] = requests;
            writer["errors"].ValueWithLabels(errors, {"kind", "timeout"});
        },
        builder
    );

    EXPECT_EQ(builder.GetMessage(), "requests\t\t10\nerrors\tkind=timeout\t2\n");
    /// [VisitMetrics custom format usage]
}

USERVER_NAMESPACE_END
