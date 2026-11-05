#include <memory>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>

#include <userver/utils/statistics/prometheus.hpp>

using USERVER_NAMESPACE::utils::statistics::ToPrometheusFormatUntyped;
using USERVER_NAMESPACE::utils::statistics::Writer;

USERVER_NAMESPACE_BEGIN

namespace {

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

USERVER_NAMESPACE_END
