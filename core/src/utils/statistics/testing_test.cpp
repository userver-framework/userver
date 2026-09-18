#include <userver/utils/statistics/testing.hpp>

#include <atomic>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/storage.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct SampleMetrics {
    std::atomic<int> foo{1};
    std::atomic<int> bar{2};
    mutable int dump_count{0};
};

void DumpMetric(utils::statistics::Writer& writer, const SampleMetrics& metrics) {
    ++metrics.dump_count;
    writer["foo"] = metrics.foo;
    writer["bar"].ValueWithLabels(metrics.bar, {"kind", "x"});
}

struct LeafMetric {
    int value{7};
};

void DumpMetric(utils::statistics::Writer& writer, const LeafMetric& metric) {
    writer.ValueWithLabels(metric.value, {"kind", "x"});
}

}  // namespace

UTEST(Snapshot, Printable) {
    const utils::statistics::Storage storage;
    const utils::statistics::Snapshot snapshot{storage};

    EXPECT_TRUE(true) << testing::PrintToString(snapshot);
}

UTEST(Snapshot, FromMetric) {
    const SampleMetrics metrics;

    const utils::statistics::Snapshot snapshot{metrics};

    EXPECT_EQ(metrics.dump_count, 1);
    EXPECT_EQ(snapshot.SingleMetric("foo").AsInt(), 1);
    EXPECT_EQ(snapshot.SingleMetric("bar", {{"kind", "x"}}).AsInt(), 2);
    EXPECT_FALSE(snapshot.SingleMetricOptional("missing").has_value());
}

UTEST(Snapshot, FromLeafMetric) {
    const utils::statistics::Snapshot snapshot{LeafMetric{}};

    EXPECT_EQ(snapshot.SingleMetric({}, {{"kind", "x"}}).AsInt(), 7);
}

UTEST(Snapshot, FromMetricPrefix) {
    const SampleMetrics metrics;
    const utils::statistics::Snapshot snapshot{metrics, "foo"};

    EXPECT_EQ(snapshot.SingleMetric({}).AsInt(), 1);
    EXPECT_FALSE(snapshot.SingleMetricOptional("bar", {{"kind", "x"}}).has_value());
}

UTEST(Snapshot, FromMetricRequireLabels) {
    const SampleMetrics metrics;
    const utils::statistics::Snapshot snapshot{metrics, {}, {{"kind", "x"}}};

    EXPECT_EQ(snapshot.SingleMetric("bar").AsInt(), 2);
    EXPECT_FALSE(snapshot.SingleMetricOptional("foo").has_value());
}

USERVER_NAMESPACE_END
