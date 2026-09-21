#include <userver/utils/statistics/testing.hpp>

#include <atomic>
#include <cstdint>
#include <optional>

#include <gtest/gtest.h>

#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

/// [metrics Snapshot DumpMetric sample]
namespace {

struct SampleMetrics {
    std::atomic<int> foo{1};
    std::atomic<int> bar{2};
};

void DumpMetric(utils::statistics::Writer& writer, const SampleMetrics& metrics) {
    writer["foo"] = metrics.foo;
    writer["bar"].ValueWithLabels(metrics.bar, {"kind", "x"});
}

}  // namespace

TEST(Snapshot, FromMetric) {
    const SampleMetrics metrics;

    const utils::statistics::Snapshot snapshot{metrics};

    EXPECT_EQ(snapshot.SingleMetric("foo"), std::int64_t{1});
    EXPECT_EQ(snapshot.SingleMetric("bar", {{"kind", "x"}}), std::int64_t{2});
    EXPECT_EQ(snapshot.SingleMetricOptional("missing"), std::nullopt);
}
/// [metrics Snapshot DumpMetric sample]

namespace {

struct LeafMetric {
    int value{7};
};

void DumpMetric(utils::statistics::Writer& writer, const LeafMetric& metric) {
    writer.ValueWithLabels(metric.value, {"kind", "x"});
}

}  // namespace

TEST(Snapshot, FromLeafMetric) {
    const utils::statistics::Snapshot snapshot{LeafMetric{}};

    EXPECT_EQ(snapshot.SingleMetric({}, {{"kind", "x"}}), std::int64_t{7});
}

TEST(Snapshot, FromMetricPrefix) {
    const SampleMetrics metrics;
    const utils::statistics::Snapshot snapshot{metrics, "foo"};

    EXPECT_EQ(snapshot.SingleMetric({}), std::int64_t{1});
    EXPECT_EQ(snapshot.SingleMetricOptional("bar", {{"kind", "x"}}), std::nullopt);
}

TEST(Snapshot, FromMetricRequireLabels) {
    const SampleMetrics metrics;
    const utils::statistics::Snapshot snapshot{metrics, {}, {{"kind", "x"}}};

    EXPECT_EQ(snapshot.SingleMetric("bar"), std::int64_t{2});
    EXPECT_EQ(snapshot.SingleMetricOptional("foo"), std::nullopt);
}

USERVER_NAMESPACE_END
