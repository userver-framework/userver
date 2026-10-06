#include <userver/utils/statistics/testing.hpp>

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <gtest/gtest.h>

#include <userver/utils/statistics/histogram.hpp>
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

namespace {

constexpr double kHistogramBounds[] = {5, 10, 20, 35, 60, 100, 173, 300, 520, 1000, 3200, 10000, 32000, 100000};

constexpr std::string_view kQueryNames[] = {"a", "b", "c", "d", "e", "f", "g", "h"};

struct QueryMetrics {
    std::unordered_map<std::string, utils::statistics::Histogram> timings;
};

void DumpMetric(utils::statistics::Writer& writer, const QueryMetrics& metrics) {
    for (const auto& [name, histogram] : metrics.timings) {
        writer["timings"].ValueWithLabels(histogram, {"query", name});
    }
}

// Mimics drivers that build a metrics snapshot on the fly and free it once the dump is over.
struct TemporaryQueryMetrics {};

void DumpMetric(utils::statistics::Writer& writer, const TemporaryQueryMetrics&) {
    QueryMetrics metrics;
    for (const auto name : kQueryNames) {
        auto [it, _] = metrics.timings.try_emplace(std::string{name}, kHistogramBounds);
        it->second.Account(42);
    }
    writer = metrics;
}

}  // namespace

TEST(Snapshot, HistogramOutlivesItsWriter) {
    const utils::statistics::Snapshot snapshot{TemporaryQueryMetrics{}};

    for (const auto name : kQueryNames) {
        EXPECT_EQ(snapshot.SingleMetric("timings", {{"query", std::string{name}}}).AsHistogram().GetTotalCount(), 1)
            << name;
    }
}

USERVER_NAMESPACE_END
