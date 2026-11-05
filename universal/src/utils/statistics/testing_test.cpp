#include <userver/utils/statistics/testing.hpp>

#include <atomic>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/statistics/histogram.hpp>
#include <userver/utils/statistics/rate.hpp>
#include <userver/utils/statistics/writer.hpp>

USERVER_NAMESPACE_BEGIN

/// [metrics Snapshot DumpMetric sample]
namespace {

struct SampleMetrics {
    std::atomic<int> foo{1};
    std::atomic<int> bar{2};
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const SampleMetrics& metrics) {
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

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const LeafMetric& metric) {
    writer.ValueWithLabels(metric.value, {"kind", "x"});
}

}  // namespace

TEST(Snapshot, FromLeafMetric) {
    const utils::statistics::Snapshot snapshot{LeafMetric{}};

    EXPECT_EQ(snapshot.SingleMetric({}, {{"kind", "x"}}), std::int64_t{7});
}

TEST(Snapshot, CopyAndMoveAssignment) {
    const utils::statistics::Snapshot source{SampleMetrics{}};
    utils::statistics::Snapshot copied{[](utils::statistics::Writer& writer) { writer["old"] = std::int64_t{42}; }};
    EXPECT_EQ(&(copied = source), &copied);
    EXPECT_THAT(copied, EqualsNumerically(source));
    EXPECT_EQ(copied.SingleMetricOptional("old"), std::nullopt);

    utils::statistics::Snapshot moved{[](utils::statistics::Writer& writer) { writer["old"] = std::int64_t{42}; }};
    static_assert(noexcept(moved = std::move(copied)));
    EXPECT_EQ(&(moved = std::move(copied)), &moved);
    EXPECT_THAT(moved, EqualsNumerically(source));
    EXPECT_EQ(moved.SingleMetricOptional("old"), std::nullopt);
}

TEST(Snapshot, FromMetricPrefix) {
    const SampleMetrics metrics;
    const utils::statistics::Snapshot snapshot{metrics, "foo"};

    EXPECT_EQ(snapshot.SingleMetric({}), std::int64_t{1});
    EXPECT_EQ(snapshot.SingleMetricOptional("bar", {{"kind", "x"}}), std::nullopt);

    const utils::statistics::Snapshot only_foo{[](utils::statistics::Writer& writer) { writer = std::int64_t{1}; }};
    EXPECT_THAT(snapshot, EqualsNumerically(only_foo));
}

TEST(Snapshot, FromMetricRequireLabels) {
    const SampleMetrics metrics;
    const utils::statistics::Snapshot snapshot{metrics, {}, {{"kind", "x"}}};

    EXPECT_EQ(snapshot.SingleMetric("bar"), std::int64_t{2});
    EXPECT_EQ(snapshot.SingleMetricOptional("foo"), std::nullopt);
}

TEST(Snapshot, BuildOptionsForMetricAndWriter) {
    const SampleMetrics metrics;
    const auto writer = [&metrics](utils::statistics::Writer& out) { out = metrics; };
    const utils::statistics::SnapshotBuildOptions cases[] = {
        {},
        {.require_prefix = "foo"},
        {.require_labels = {{"kind", "x"}}},
        {.require_prefix = "bar", .require_labels = {{"kind", "x"}}},
        {.require_prefix = "foo", .require_labels = {{"kind", "x"}}},
        {.require_prefix = "bar", .require_labels = {{"kind", "y"}}},
    };
    for (const auto& options : cases) {
        SCOPED_TRACE(testing::PrintToString(options.require_prefix));
        const utils::statistics::Snapshot from_metric{metrics, options};
        const utils::statistics::Snapshot from_writer{writer, options};
        const utils::statistics::Snapshot
            legacy_metric{metrics, options.require_prefix.value_or(""), options.require_labels};
        const utils::statistics::Snapshot
            legacy_writer{writer, options.require_prefix.value_or(""), options.require_labels};
        EXPECT_THAT(from_metric, EqualsNumerically(legacy_metric));
        EXPECT_THAT(from_writer, EqualsNumerically(legacy_writer));
        EXPECT_THAT(from_writer, EqualsNumerically(from_metric));
    }

    const utils::statistics::Snapshot
        from_metric{metrics, {.require_prefix = "bar", .require_labels = {{"kind", "x"}}}};
    const utils::statistics::Snapshot from_writer{writer, {.require_prefix = "bar", .require_labels = {{"kind", "x"}}}};
    EXPECT_EQ(from_metric.SingleMetric({}), std::int64_t{2});
    EXPECT_EQ(from_writer.SingleMetric({}), std::int64_t{2});
    EXPECT_THAT(utils::statistics::Snapshot{writer}, EqualsNumerically(utils::statistics::Snapshot{metrics}));
}

TEST(Snapshot, EmptyBuildPrefixIsRejectedBeforeDump) {
    for (const bool sliced : {false, true}) {
        SCOPED_TRACE(sliced);
        const utils::statistics::SnapshotBuildOptions options{.require_prefix = "", .sliced = sliced};
        const auto writer = [](utils::statistics::Writer&) { ADD_FAILURE() << "Writer must not be called"; };
        EXPECT_THROW((utils::statistics::Snapshot{writer, options}), std::invalid_argument);
        EXPECT_THROW((utils::statistics::Snapshot{SampleMetrics{}, options}), std::invalid_argument);
    }
}

TEST(Snapshot, MissingBuildPrefixPreservesPaths) {
    const auto writer = [](utils::statistics::Writer& out) {
        out = std::int64_t{1};
        out[".requests"] = std::int64_t{2};
        out["api.requests"] = std::int64_t{3};
    };
    for (const bool sliced : {false, true}) {
        SCOPED_TRACE(sliced);
        const utils::statistics::Snapshot snapshot{writer, {.require_prefix = std::nullopt, .sliced = sliced}};
        EXPECT_EQ(snapshot.SingleMetric(""), std::int64_t{1});
        EXPECT_EQ(snapshot.SingleMetric(".requests"), std::int64_t{2});
        EXPECT_EQ(snapshot.SingleMetric("api.requests"), std::int64_t{3});
        EXPECT_THAT(snapshot, EqualsNumerically(utils::statistics::Snapshot{writer}));
    }
}

TEST(Snapshot, DumpMetricUsesStoredSlicedPathAndLabels) {
    const utils::statistics::Snapshot snapshot{
        [](utils::statistics::Writer& writer) {
            writer["checkout.orders"].ValueWithLabels(std::int64_t{4}, {{"status", "paid"}, {"region", "eu"}});
            writer["checkout.refunds"] = std::int64_t{1};
            writer["billing.invoices"] = std::int64_t{1};
        },
        "checkout",
        {{"status", "paid"}},
    };

    EXPECT_EQ(snapshot.SingleMetric("orders", {{"region", "eu"}}), std::int64_t{4});
    EXPECT_EQ(snapshot.SingleMetricOptional("checkout.orders", {{"status", "paid"}}), std::nullopt);
    EXPECT_EQ(snapshot.SingleMetricOptional("refunds"), std::nullopt);

    const utils::statistics::Snapshot dumped{[&snapshot](utils::statistics::Writer& writer) { writer = snapshot; }};
    EXPECT_EQ(dumped.SingleMetric("orders", {{"region", "eu"}}), std::int64_t{4});
    EXPECT_EQ(dumped.SingleMetricOptional("checkout.orders", {{"status", "paid"}}), std::nullopt);

    const utils::statistics::Snapshot expected{[](utils::statistics::Writer& writer) {
        writer["orders"].ValueWithLabels(std::int64_t{4}, {{"region", "eu"}});
    }};
    EXPECT_THAT(dumped, EqualsNumerically(expected));
}

TEST(Snapshot, EqualityComparesNumericValues) {
    const utils::statistics::Snapshot collected{[](utils::statistics::Writer& writer) {
        writer["in_flight"] = std::int64_t{4};
        writer["queries"] = utils::statistics::Rate{3};
    }};
    const auto reference = utils::statistics::Snapshot::FromJson(formats::json::FromString(R"({
        "in_flight": [{"type": "GAUGE", "value": 4, "labels": {}}],
        "queries": [{"type": "GAUGE", "value": 3, "labels": {}}]
    })"));

    EXPECT_THAT(collected, EqualsNumerically(reference));

    const utils::statistics::Snapshot other_count{[](utils::statistics::Writer& writer) {
        writer["in_flight"] = std::int64_t{5};
        writer["queries"] = utils::statistics::Rate{3};
    }};
    EXPECT_THAT(collected, testing::Not(EqualsNumerically(other_count)));
}

TEST(Snapshot, EqualityUsesStoredSlicedPathAndIgnoresLabelOrder) {
    const utils::statistics::Snapshot collected{
        [](utils::statistics::Writer& writer) {
            writer["handler.requests"]
                .ValueWithLabels(std::int64_t{120}, {{"handler", "/orders"}, {"http_code", "200"}});
        },
        "handler",
    };
    const auto reference = utils::statistics::Snapshot::FromJson(formats::json::FromString(R"({
        "requests": [{
            "type": "GAUGE",
            "value": 120,
            "labels": {"http_code": "200", "handler": "/orders"}
        }]
    })"));

    EXPECT_THAT(collected, EqualsNumerically(reference));
}

namespace {

constexpr double kHistogramBounds[] = {5, 10, 20, 35, 60, 100, 173, 300, 520, 1000, 3200, 10000, 32000, 100000};

constexpr std::string_view kQueryNames[] = {
    "select_order",
    "insert_order",
    "update_order",
    "delete_order",
    "count_orders",
    "lock_order",
    "list_orders",
    "ping",
};

struct QueryMetrics {
    std::unordered_map<std::string, utils::statistics::Histogram> timings;
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const QueryMetrics& metrics) {
    for (const auto& [name, histogram] : metrics.timings) {
        writer["timings"].ValueWithLabels(histogram, {"query", name});
    }
}

// Mimics drivers that build a metrics snapshot on the fly and free it once the dump is over.
struct TemporaryQueryMetrics {};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const TemporaryQueryMetrics&) {
    QueryMetrics metrics;
    for (const auto name : kQueryNames) {
        auto [it, _] = metrics.timings.try_emplace(std::string{name}, kHistogramBounds);
        it->second.Account(42);
    }
    writer = metrics;
}

}  // namespace

/// [metrics Snapshot FromJson]
TEST(Snapshot, FromJson) {
    const auto snapshot = utils::statistics::Snapshot::FromJson(formats::json::FromString(R"({
        "handler.requests": [
            {
                "type": "GAUGE",
                "value": 120,
                "labels": {"handler": "/orders", "http_code": "200"}
            }
        ],
        "handler.duration_seconds": [
            {"type": "GAUGE", "value": 1.5, "labels": {}}
        ],
        "db.queries": [
            {"type": "RATE", "value": 3, "labels": {"query": "select_order"}}
        ],
        "handler.timings": [
            {
                "type": "HIST_RATE",
                "labels": {},
                "value": {"bounds": [5.0, 10.0], "buckets": [2, 3], "inf": 1, "sum": 100.0}
            }
        ]
    })"));

    EXPECT_EQ(
        snapshot.SingleMetric("handler.requests", {{"handler", "/orders"}, {"http_code", "200"}}),
        std::int64_t{120}
    );
    EXPECT_EQ(snapshot.SingleMetric("handler.duration_seconds"), 1.5);
    EXPECT_EQ(snapshot.SingleMetric("db.queries", {{"query", "select_order"}}), utils::statistics::Rate{3});

    const auto timings = snapshot.SingleMetric("handler.timings").AsHistogram();
    EXPECT_EQ(timings.GetValueAt(0), 2);
    EXPECT_EQ(timings.GetValueAt(1), 3);
    EXPECT_EQ(timings.GetValueAtInf(), 1);
    EXPECT_EQ(timings.GetTotalSum(), 100.0);
}
/// [metrics Snapshot FromJson]

namespace {

struct GaugeNumberCase {
    std::string_view json_number;
    utils::statistics::MetricValue expected;
};

class SnapshotGaugeNumber : public testing::TestWithParam<GaugeNumberCase> {};

}  // namespace

TEST_P(SnapshotGaugeNumber, FlatJson) {
    const auto& test = GetParam();
    const auto json = formats::json::FromString(
        std::string{R"({"metric":[{"type":"GAUGE","labels":{},"value":)"} + std::string{test.json_number} + "}]}"
    );
    const auto snapshot = utils::statistics::Snapshot::FromJson(json);
    EXPECT_EQ(snapshot.SingleMetric("metric"), test.expected);
}

TEST_P(SnapshotGaugeNumber, LayeredJson) {
    const auto& test = GetParam();
    const auto number = std::string{test.json_number};
    const auto json =
        formats::json::FromString("{\"plain\":" + number + R"(,"typed":{"type":"GAUGE","value":)" + number + "}}");
    const auto snapshot = utils::statistics::Snapshot::FromLayeredJson(json);
    EXPECT_EQ(snapshot.SingleMetric("plain"), test.expected);
    EXPECT_EQ(snapshot.SingleMetric("typed"), test.expected);
    const utils::statistics::Snapshot expected{[&test](utils::statistics::Writer& writer) {
        writer["plain"] = test.expected;
        writer["typed"] = test.expected;
    }};
    EXPECT_THAT(snapshot, EqualsNumerically(expected));
}

INSTANTIATE_TEST_SUITE_P(
    Snapshot,
    SnapshotGaugeNumber,
    testing::Values(
        GaugeNumberCase{"4.0", std::int64_t{4}},
        GaugeNumberCase{"1.5", 1.5},
        GaugeNumberCase{"9007199254740993", std::int64_t{9007199254740993}},
        GaugeNumberCase{"-9007199254740993", std::int64_t{-9007199254740993}},
        GaugeNumberCase{"9223372036854775807", std::int64_t{9223372036854775807}},
        GaugeNumberCase{"-9223372036854775808", std::int64_t{-9223372036854775807 - 1}},
        GaugeNumberCase{"9223372036854775808", 0x1p63},
        GaugeNumberCase{"18446744073709551615", 0x1p64}
    )
);

TEST(Snapshot, FromJsonLoadOptions) {
    const auto snapshot = utils::statistics::Snapshot::FromJson(
        formats::json::FromString(R"({
            "requests": [
                {"type": "GAUGE", "value": 120, "labels": {"handler": "/orders"}}
            ],
            "queries": [
                {"type": "RATE", "value": 3, "labels": {"query": "select_order"}}
            ]
        })"),
        utils::statistics::SnapshotLoadOptions{
            .add_prefix = "api",
            .add_labels = {utils::statistics::Label{"service", "orders"}},
        }
    );

    EXPECT_EQ(
        snapshot.SingleMetric("api.requests", {{"handler", "/orders"}, {"service", "orders"}}),
        std::int64_t{120}
    );
    EXPECT_EQ(
        snapshot.SingleMetric("api.queries", {{"query", "select_order"}, {"service", "orders"}}),
        utils::statistics::Rate{3}
    );
    EXPECT_EQ(snapshot.SingleMetricOptional("requests"), std::nullopt);
}

TEST(Snapshot, FromLayeredJsonEmptyLabelLevels) {
    // An empty object is a label level with zero key-value alternatives, not a malformed metric value.
    for (const auto* json : {R"({"requests":{}})", R"({"requests":{"handler = /orders":{}}})"}) {
        SCOPED_TRACE(json);
        const auto snapshot = utils::statistics::Snapshot::FromLayeredJson(formats::json::FromString(json));
        EXPECT_TRUE(snapshot.AllMetrics().empty());
    }

    // Empty branches must not suppress siblings or leak their labels into them.
    const auto snapshot = utils::statistics::Snapshot::FromLayeredJson(formats::json::FromString(R"({
        "requests": {"handler = /orders": {}, "handler = /status": 3},
        "empty": {},
        "other": 4
    })"));
    const utils::statistics::Snapshot expected{[](utils::statistics::Writer& writer) {
        writer["requests"].ValueWithLabels(std::int64_t{3}, {"handler", "/status"});
        writer["other"] = std::int64_t{4};
    }};
    EXPECT_THAT(snapshot, EqualsNumerically(expected));
}

TEST(Snapshot, FromLayeredJson) {
    const auto snapshot = utils::statistics::Snapshot::FromLayeredJson(
        formats::json::FromString(R"({
            "requests": {"handler = /orders": {"http_code = 200": 120, "http_code = 500": 1}},
            "queries": {"query = select_order": {"type": "RATE", "value": 3}},
            "in_flight": {"type": "GAUGE", "value": 1.5},
            "empty_label": {"handler = ": 2},
            "timings": {"bounds": [5.0, 10.0], "buckets": [2, 3], "inf": 1, "sum": 100.5},
            "typed_timings": {
                "type": "HIST_RATE",
                "value": {"bounds": [5.0, 10.0], "buckets": [2, 3], "inf": 1}
            }
        })"),
        utils::statistics::SnapshotLoadOptions{
            .add_prefix = "api",
            .add_labels = {{"service", "orders"}},
        }
    );
    const auto flat = utils::statistics::Snapshot::FromJson(formats::json::FromString(R"({
        "api.requests": [
            {"type": "GAUGE", "value": 120, "labels": {"handler": "/orders", "http_code": "200", "service": "orders"}},
            {"type": "GAUGE", "value": 1, "labels": {"handler": "/orders", "http_code": "500", "service": "orders"}}
        ],
        "api.queries": [{"type": "RATE", "value": 3, "labels": {"query": "select_order", "service": "orders"}}],
        "api.in_flight": [{"type": "GAUGE", "value": 1.5, "labels": {"service": "orders"}}],
        "api.empty_label": [{"type": "GAUGE", "value": 2, "labels": {"handler": "", "service": "orders"}}],
        "api.timings": [{"type": "HIST_RATE", "labels": {"service": "orders"},
            "value": {"bounds": [5.0, 10.0], "buckets": [2, 3], "inf": 1, "sum": 100.5}}],
        "api.typed_timings": [{"type": "HIST_RATE", "labels": {"service": "orders"},
            "value": {"bounds": [5.0, 10.0], "buckets": [2, 3], "inf": 1}}]
    })"));
    EXPECT_THAT(snapshot, EqualsNumerically(flat));
    EXPECT_EQ(snapshot.SingleMetric("api.in_flight", {{"service", "orders"}}).AsFloat(), 1.5);
    EXPECT_EQ(snapshot.SingleMetricOptional("requests"), std::nullopt);
    // Histogram equality compares bounds and counts, not sums.
    EXPECT_EQ(snapshot.SingleMetric("api.timings").AsHistogram().GetTotalSum(), 100.5);
    EXPECT_EQ(snapshot.SingleMetric("api.typed_timings").AsHistogram().GetTotalSum(), 0.0);
}

/// [metrics Snapshot EqualsNumerically]
namespace {

struct DbMetrics {
    utils::statistics::Rate queries{0};
};

struct HandlerMetrics {
    std::int64_t requests{0};
    double duration_seconds{0};
};

struct ServiceMetrics {
    HandlerMetrics handler;
    DbMetrics db;
};

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const DbMetrics& metrics) {
    writer["queries"].ValueWithLabels(metrics.queries, {"query", "select_order"});
}

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const HandlerMetrics& metrics) {
    writer["requests"].ValueWithLabels(metrics.requests, {"http_code", "200"});
    writer["duration_seconds"] = metrics.duration_seconds;
}

[[maybe_unused]] void DumpMetric(utils::statistics::Writer& writer, const ServiceMetrics& metrics) {
    writer["handler"] = metrics.handler;
    writer["db"] = metrics.db;
}

static_assert(utils::statistics::HasWriterSupport<ServiceMetrics>);

}  // namespace

TEST(Snapshot, NestedMetricsMatchLayeredReference) {
    ServiceMetrics metrics;
    metrics.handler.requests = 120;
    metrics.handler.duration_seconds = 1.5;
    metrics.db.queries = utils::statistics::Rate{3};

    const utils::statistics::Snapshot snapshot{metrics};
    const auto reference = utils::statistics::Snapshot::FromLayeredJson(formats::json::FromString(R"({
        "handler.requests": {"http_code = 200": 120},
        "handler.duration_seconds": 1.5,
        "db.queries": {"query = select_order": 3}
    })"));
    EXPECT_THAT(snapshot, EqualsNumerically(reference));
}
/// [metrics Snapshot EqualsNumerically]

namespace {

struct SlicingMetrics {};

void DumpMetric(utils::statistics::Writer& writer, const SlicingMetrics&) {
    writer["api"].ValueWithLabels(std::int64_t{1}, {{"service", "orders"}, {"region", "eu"}});
    writer["api.requests"].ValueWithLabels(std::int64_t{2}, {{"service", "orders"}, {"region", "eu"}, {"code", "200"}});
    writer["api.requests.errors"]
        .ValueWithLabels(std::int64_t{3}, {{"service", "orders"}, {"region", "eu"}, {"code", "500"}});
    writer["api.requests_total"].ValueWithLabels(std::int64_t{4}, {{"service", "orders"}, {"region", "eu"}});
    writer["apis"].ValueWithLabels(std::int64_t{5}, {{"service", "orders"}, {"region", "eu"}});
    writer["other"].ValueWithLabels(std::int64_t{6}, {{"service", "orders"}, {"region", "eu"}});
}

class SnapshotSlicing : public testing::TestWithParam<bool> {
protected:
    utils::statistics::SnapshotBuildOptions Options() const {
        return {
            .require_prefix = "api",
            .require_labels = {{"service", "orders"}, {"region", "eu"}},
            .sliced = GetParam(),
        };
    }

    std::string Path(std::string_view sliced, std::string_view full) const {
        return std::string{GetParam() ? sliced : full};
    }

    std::vector<utils::statistics::Label> Labels(std::initializer_list<utils::statistics::Label> extras = {}) const {
        std::vector<utils::statistics::Label> result;
        if (!GetParam()) {
            result = {{"service", "orders"}, {"region", "eu"}};
        }
        result.insert(result.end(), extras);
        return result;
    }
};

}  // namespace

/// [metrics Snapshot sliced queries]
TEST(Snapshot, SlicedAndUnslicedQueries) {
    const auto writer = [](utils::statistics::Writer& out) {
        out["api.requests"].ValueWithLabels(std::int64_t{2}, {{"service", "orders"}, {"code", "200"}});
    };
    const utils::statistics::Snapshot
        sliced{writer, {.require_prefix = "api", .require_labels = {{"service", "orders"}}}};
    EXPECT_EQ(sliced.SingleMetric("requests", {{"code", "200"}}), std::int64_t{2});

    const utils::statistics::Snapshot
        unsliced{writer, {.require_prefix = "api", .require_labels = {{"service", "orders"}}, .sliced = false}};
    EXPECT_EQ(unsliced.SingleMetric("api.requests", {{"service", "orders"}, {"code", "200"}}), std::int64_t{2});
}
/// [metrics Snapshot sliced queries]

TEST_P(SnapshotSlicing, QueriesUseOnlyStoredPathsAndLabels) {
    const utils::statistics::Snapshot snapshot{SlicingMetrics{}, Options()};

    EXPECT_EQ(snapshot.SingleMetric(Path({}, "api"), Labels()), std::int64_t{1});
    EXPECT_EQ(snapshot.SingleMetric(Path("requests", "api.requests"), Labels({{"code", "200"}})), std::int64_t{2});
    EXPECT_EQ(
        snapshot.SingleMetricOptional(Path("requests", "api.requests"), Labels({{"code", "200"}})),
        utils::statistics::MetricValue{std::int64_t{2}}
    );
    EXPECT_EQ(snapshot.SingleMetricOptional(Path("requests", "api.requests"), Labels({{"code", "500"}})), std::nullopt);
    if (GetParam()) {
        EXPECT_EQ(snapshot.SingleMetricOptional("s"), std::nullopt);
        EXPECT_EQ(snapshot.SingleMetricOptional("apis"), std::nullopt);
    } else {
        EXPECT_EQ(snapshot.SingleMetric("apis", Labels()), std::int64_t{5});
    }
    EXPECT_EQ(snapshot.SingleMetricOptional("other", Labels()), std::nullopt);

    const auto removed_path = GetParam() ? "api.requests" : "requests";
    EXPECT_EQ(snapshot.SingleMetricOptional(removed_path, Labels({{"code", "200"}})), std::nullopt);
    EXPECT_THROW(snapshot.SingleMetric(removed_path, Labels({{"code", "200"}})), utils::statistics::MetricQueryError);
    const auto labels_with_removed = Labels({{"service", "orders"}, {"code", "200"}});
    if (GetParam()) {
        EXPECT_EQ(snapshot.SingleMetricOptional(Path("requests", "api.requests"), labels_with_removed), std::nullopt);
    } else {
        EXPECT_EQ(snapshot.SingleMetric(Path("requests", "api.requests"), labels_with_removed), std::int64_t{2});
    }
    EXPECT_EQ(snapshot.SingleMetric(Path("requests", "api.requests")), std::int64_t{2});
}

TEST(Snapshot, QueriesAllowAdditionalLabels) {
    const utils::statistics::Snapshot snapshot{[](utils::statistics::Writer& writer) {
        writer["requests"] = std::int64_t{1};
        writer["requests"].ValueWithLabels(std::int64_t{2}, {{"handler", "/orders"}, {"code", "200"}});
        writer["requests"].ValueWithLabels(std::int64_t{3}, {{"handler", "/orders"}, {"code", "500"}});
    }};

    EXPECT_EQ(snapshot.SingleMetric("requests", {{"code", "200"}}), std::int64_t{2});
    EXPECT_EQ(
        snapshot.SingleMetricOptional("requests", {{"code", "500"}}),
        utils::statistics::MetricValue{std::int64_t{3}}
    );
    EXPECT_EQ(snapshot.SingleMetricOptional("missing"), std::nullopt);
    EXPECT_THROW(snapshot.SingleMetric("missing"), utils::statistics::MetricQueryError);
    EXPECT_THROW(snapshot.SingleMetric("requests"), utils::statistics::MetricQueryError);
    EXPECT_THROW(snapshot.SingleMetricOptional("requests"), utils::statistics::MetricQueryError);
    EXPECT_THROW(snapshot.SingleMetric("requests", {{"handler", "/orders"}}), utils::statistics::MetricQueryError);
    EXPECT_THROW(
        snapshot.SingleMetricOptional("requests", {{"handler", "/orders"}}),
        utils::statistics::MetricQueryError
    );
}

TEST_P(SnapshotSlicing, WriterConstructorIterationDumpEqualityAndMatcherUseStoredMetrics) {
    const auto options = Options();
    const std::string expected_path = Path("requests", "api.requests");
    const auto expected_labels = Labels({{"code", "200"}});
    const utils::statistics::Snapshot
        snapshot{[](utils::statistics::Writer& writer) { DumpMetric(writer, SlicingMetrics{}); }, options};

    const auto metrics = snapshot.AllMetrics();
    ASSERT_EQ(metrics.size(), GetParam() ? 4 : 5);
    const auto request = std::ranges::find_if(metrics, [&expected_path](const auto& metric) {
        return metric.path == expected_path;
    });
    ASSERT_NE(request, metrics.end());
    ASSERT_EQ(request->labels.size(), expected_labels.size());
    for (const auto& label : expected_labels) {
        EXPECT_TRUE(std::ranges::any_of(request->labels, [&label](const auto& view) {
            return view.Name() == label.Name() && view.Value() == label.Value();
        }));
    }

    const utils::statistics::Snapshot dumped_snapshot{[&snapshot](utils::statistics::Writer& writer) {
        writer = snapshot;
    }};
    const utils::statistics::Snapshot dumped_view{[&request](utils::statistics::Writer& writer) {
        DumpMetric(writer, *request);
    }};
    const utils::statistics::Snapshot expected_view{
        [&expected_path, &expected_labels](utils::statistics::Writer& writer) {
            const auto label_views = std::views::transform(expected_labels, [](const auto& label) {
                return utils::statistics::LabelView{label};
            });
            const std::vector<utils::statistics::LabelView> labels{label_views.begin(), label_views.end()};
            writer[expected_path].ValueWithLabels(std::int64_t{2}, utils::statistics::LabelsSpan{labels});
        }
    };
    EXPECT_TRUE(dumped_view.EqualsNumerically(expected_view));
    EXPECT_THAT(dumped_view, EqualsNumerically(expected_view));
    EXPECT_THAT(dumped_snapshot, EqualsNumerically(snapshot));
    auto opposite_options = options;
    opposite_options.sliced = !GetParam();
    EXPECT_FALSE(snapshot.EqualsNumerically(utils::statistics::Snapshot{SlicingMetrics{}, opposite_options}));
    EXPECT_FALSE(testing::PrintToString(snapshot).empty());
}

INSTANTIATE_TEST_SUITE_P(Snapshot, SnapshotSlicing, testing::Bool());

TEST(Snapshot, SlicingRejectsTrailingDotPrefixBeforeDump) {
    const auto writer = [](utils::statistics::Writer&) { ADD_FAILURE() << "Writer must not be called"; };
    const utils::statistics::SnapshotBuildOptions options{.require_prefix = "api.", .sliced = true};
    EXPECT_THROW((utils::statistics::Snapshot{writer, options}), std::invalid_argument);
    EXPECT_THROW((utils::statistics::Snapshot{SampleMetrics{}, options}), std::invalid_argument);
    EXPECT_THROW((utils::statistics::Snapshot{writer, "api."}), std::invalid_argument);
}

TEST(Snapshot, SlicingRejectsAmbiguousEmptyPath) {
    const auto writer = [](utils::statistics::Writer& out) { out["api."] = std::int64_t{2}; };
    try {
        const utils::statistics::Snapshot snapshot{writer, {.require_prefix = "api"}};
        FAIL() << "Expected ambiguous empty path to throw";
    } catch (const std::invalid_argument& exception) {
        EXPECT_THAT(exception.what(), testing::HasSubstr("indistinguishable from path 'api'"));
    }
    for (const auto& prefix : {"api", "api."}) {
        const utils::statistics::Snapshot snapshot{writer, {.require_prefix = prefix, .sliced = false}};
        EXPECT_EQ(snapshot.SingleMetric("api."), std::int64_t{2});
    }
    const utils::statistics::Snapshot unfiltered{writer};
    EXPECT_EQ(unfiltered.SingleMetric("api."), std::int64_t{2});
}

TEST(Snapshot, SlicingPreservesEmptyInnerSegment) {
    const utils::statistics::Snapshot snapshot{
        [](utils::statistics::Writer& out) {
            out["api"] = std::int64_t{1};
            out["api..requests"] = std::int64_t{2};
        },
        {.require_prefix = "api"},
    };
    EXPECT_EQ(snapshot.SingleMetric(""), std::int64_t{1});
    EXPECT_EQ(snapshot.SingleMetric(".requests"), std::int64_t{2});
}

TEST(Snapshot, SlicingOwnsSourceAndOptionsData) {
    const utils::statistics::Snapshot snapshot = [] {
        const SlicingMetrics metrics;
        const utils::statistics::SnapshotBuildOptions options{
            .require_prefix = std::string{"api"},
            .require_labels =
                {{std::string{"service"}, std::string{"orders"}}, {std::string{"region"}, std::string{"eu"}}},
        };
        return utils::statistics::Snapshot{metrics, options};
    }();

    EXPECT_EQ(snapshot.SingleMetric("requests", {{"code", "200"}}), std::int64_t{2});
    EXPECT_FALSE(testing::PrintToString(snapshot).empty());
}

TEST(Snapshot, LegacyConstructorsSliceForMetricAndWriter) {
    const SlicingMetrics metrics;
    const auto writer = [&metrics](utils::statistics::Writer& out) { out = metrics; };
    const utils::statistics::Snapshot from_metric{metrics, "api", {{"service", "orders"}, {"region", "eu"}}};
    const utils::statistics::Snapshot from_writer{writer, "api", {{"service", "orders"}, {"region", "eu"}}};
    EXPECT_EQ(from_metric.SingleMetric("requests", {{"code", "200"}}), std::int64_t{2});
    EXPECT_THAT(from_writer, EqualsNumerically(from_metric));
}

TEST(Snapshot, FromJsonDuplicateCommonLabelThrows) {
    UEXPECT_THROW(
        utils::statistics::Snapshot::FromJson(
            formats::json::FromString(R"({
                "requests": [{"type": "GAUGE", "value": 1, "labels": {"handler": "/orders"}}]
            })"),
            utils::statistics::SnapshotLoadOptions{
                .add_labels = {utils::statistics::Label{"handler", "/list"}},
            }
        ),
        utils::statistics::DuplicateLabelError
    );
}

TEST(Snapshot, LayeredDuplicateLabelThrows) {
    UEXPECT_THROW(
        utils::statistics::Snapshot::FromLayeredJson(
            formats::json::FromString(R"({"requests": {"handler = /orders": 1}})"),
            utils::statistics::SnapshotLoadOptions{
                .add_labels = {utils::statistics::Label{"handler", "/list"}},
            }
        ),
        utils::statistics::DuplicateLabelError
    );
}

TEST(Snapshot, DuplicateLabelsValidatedBeforeSlicing) {
    EXPECT_THROW(
        (utils::statistics::Snapshot{
            [](utils::statistics::Writer& writer) {
                writer["requests"].ValueWithLabels(std::int64_t{1}, {{"handler", "/orders"}, {"handler", "/list"}});
            },
            {.require_labels = {{"handler", "/orders"}}, .sliced = true},
        }),
        utils::statistics::DuplicateLabelError
    );
}

namespace {

struct MalformedCase {
    std::string_view json;
    std::string_view path;
    std::optional<std::string_view> message;
};

void CheckMalformedException(const formats::json::ExceptionWithPath& exception, const MalformedCase& malformed) {
    EXPECT_EQ(exception.GetPath(), malformed.path);
    if (malformed.message) {
        EXPECT_EQ(exception.GetMessageWithoutPath(), malformed.message.value());
    }
}

class FromJsonMalformed : public testing::TestWithParam<MalformedCase> {};
class FromLayeredJsonMalformed : public testing::TestWithParam<MalformedCase> {};

}  // namespace

TEST_P(FromJsonMalformed, ThrowsWithPath) {
    const auto& malformed = GetParam();
    try {
        [[maybe_unused]] const auto
            snapshot = utils::statistics::Snapshot::FromJson(formats::json::FromString(malformed.json));
        FAIL() << "Expected malformed metrics JSON to throw";
    } catch (const formats::json::ExceptionWithPath& exception) {
        CheckMalformedException(exception, malformed);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Snapshot,
    FromJsonMalformed,
    testing::Values(
        MalformedCase{
            R"({"duration_seconds":[{"type":"GAUGE","value":"slow","labels":{}}]})",
            "duration_seconds[0].value",
            std::nullopt
        },
        MalformedCase{R"({"queries":[{"type":"RATE","value":-1,"labels":{}}]})", "queries[0].value", std::nullopt},
        MalformedCase{R"({"queries":[{"type":"RATE","value":1.5,"labels":{}}]})", "queries[0].value", std::nullopt},
        MalformedCase{
            R"({"timings":[{"type":"HIST_RATE","labels":{},"value":{"bounds":[1,2],"buckets":[1],"inf":0}}]})",
            "timings[0].value",
            "Histogram buckets size does not match bounds"
        },
        MalformedCase{
            R"({"timings":[{"type":"HIST_RATE","labels":{},"value":{"bounds":[0],"buckets":[0],"inf":0}}]})",
            "timings[0].value.bounds",
            "Histogram bounds must be positive normal numbers"
        },
        MalformedCase{
            R"({"timings":[{"type":"HIST_RATE","labels":{},"value":{"bounds":[-1],"buckets":[0],"inf":0}}]})",
            "timings[0].value.bounds",
            "Histogram bounds must be positive normal numbers"
        },
        MalformedCase{
            R"({"timings":[{"type":"HIST_RATE","labels":{},"value":{"bounds":[1e-310],"buckets":[0],"inf":0}}]})",
            "timings[0].value.bounds",
            "Histogram bounds must be positive normal numbers"
        },
        MalformedCase{
            R"({"timings":[{"type":"HIST_RATE","labels":{},"value":{"bounds":[2,1],"buckets":[0,0],"inf":0}}]})",
            "timings[0].value.bounds",
            "Histogram bounds must be sorted"
        },
        MalformedCase{
            R"({"timings":[{"type":"HIST_RATE","labels":{},"value":{"bounds":[1,1],"buckets":[0,0],"inf":0}}]})",
            "timings[0].value.bounds",
            "Histogram bounds must not contain duplicates"
        },
        MalformedCase{
            R"({"timings":[{"type":"HIST_RATE","labels":{},"value":{"bounds":[5],"buckets":[1]}}]})",
            "timings[0].value.inf",
            std::nullopt
        },
        MalformedCase{
            R"({"requests":[{"type":"COUNTER","value":1,"labels":{}}]})",
            "requests[0].value",
            "Unknown metric type 'COUNTER'"
        },
        MalformedCase{"[]", "/", std::nullopt},
        MalformedCase{R"({"in_flight":{"type":"GAUGE","value":1}})", "in_flight", std::nullopt}
    )
);

TEST_P(FromLayeredJsonMalformed, ThrowsWithPath) {
    const auto& malformed = GetParam();
    try {
        [[maybe_unused]] const auto
            snapshot = utils::statistics::Snapshot::FromLayeredJson(formats::json::FromString(malformed.json));
        FAIL() << "Expected malformed layered metrics JSON to throw";
    } catch (const formats::json::ExceptionWithPath& exception) {
        CheckMalformedException(exception, malformed);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Snapshot,
    FromLayeredJsonMalformed,
    testing::Values(
        MalformedCase{
            R"({"duration_seconds":{"type":"GAUGE","value":"slow"}})",
            "duration_seconds.value",
            std::nullopt
        },
        MalformedCase{R"({"queries":{"type":"RATE","value":-1}})", "queries.value", std::nullopt},
        MalformedCase{R"({"queries":{"type":"RATE","value":1.5}})", "queries.value", std::nullopt},
        MalformedCase{
            R"({"timings":{"bounds":[1,2],"buckets":[1],"inf":0}})",
            "timings",
            "Histogram buckets size does not match bounds"
        },
        MalformedCase{
            R"({"requests":{"type":"COUNTER","value":1}})",
            "requests.value",
            "Unknown metric type 'COUNTER'"
        },
        MalformedCase{
            R"({"timings":{"bounds":[0],"buckets":[0],"inf":0}})",
            "timings.bounds",
            "Histogram bounds must be positive normal numbers"
        },
        MalformedCase{
            R"({"timings":{"bounds":[-1],"buckets":[0],"inf":0}})",
            "timings.bounds",
            "Histogram bounds must be positive normal numbers"
        },
        MalformedCase{
            R"({"timings":{"bounds":[1e-310],"buckets":[0],"inf":0}})",
            "timings.bounds",
            "Histogram bounds must be positive normal numbers"
        },
        MalformedCase{
            R"({"timings":{"bounds":[2,1],"buckets":[0,0],"inf":0}})",
            "timings.bounds",
            "Histogram bounds must be sorted"
        },
        MalformedCase{
            R"({"timings":{"bounds":[1,1],"buckets":[0,0],"inf":0}})",
            "timings.bounds",
            "Histogram bounds must not contain duplicates"
        },
        MalformedCase{
            R"({"timings":{"type":"HIST_RATE","value":{"bounds":[2,1],"buckets":[0,0],"inf":0}}})",
            "timings.value.bounds",
            "Histogram bounds must be sorted"
        },
        MalformedCase{R"({"timings":{"bounds":[5],"buckets":[1]}})", "timings.inf", std::nullopt},
        MalformedCase{
            R"({"in_flight":[{"type":"GAUGE","value":4,"labels":{"handler":"/orders"}}]})",
            "in_flight[0].labels",
            "Field 'labels' belongs to the FromJson format. Use Snapshot::FromJson"
        },
        MalformedCase{"[]", "/", std::nullopt},
        MalformedCase{
            R"({"requests":{"handler":1}})",
            "requests.handler",
            "Expected a label key like 'name = value', got 'handler'"
        },
        MalformedCase{
            R"({"requests":{"handler=/orders":1}})",
            "requests.handler=/orders",
            "Expected a label key like 'name = value', got 'handler=/orders'"
        },
        MalformedCase{
            R"({"requests":{"handler  = /orders":1}})",
            "requests.handler  = /orders",
            "Expected exactly one space on each side of '=' in a label key, got 'handler  = /orders'"
        },
        MalformedCase{
            R"({"requests":{"handler =  /orders":1}})",
            "requests.handler =  /orders",
            "Expected exactly one space on each side of '=' in a label key, got 'handler =  /orders'"
        },
        MalformedCase{
            R"({"requests":{" = /orders":1}})",
            "requests. = /orders",
            "Label name must not be empty, got ' = /orders'"
        }
    )
);

TEST(Snapshot, AllMetricsRoundtripPreservesStoredPathsAndLabelSets) {
    const utils::statistics::Snapshot empty{[](utils::statistics::Writer& /*writer*/) {}};
    EXPECT_TRUE(empty.AllMetrics().empty());

    const utils::statistics::Snapshot snapshot{
        [](utils::statistics::Writer& writer) {
            writer["api.series"] = std::int64_t{1};
            writer["api.series"].ValueWithLabels(std::int64_t{2}, {"kind", "a"});
            writer["api.series"].ValueWithLabels(std::int64_t{3}, {"kind", "b"});
        },
        "api",
    };

    const auto metrics = snapshot.AllMetrics();
    ASSERT_EQ(metrics.size(), 3);
    for (const auto metric : metrics) {
        EXPECT_EQ(metric.path, "series");
    }

    const utils::statistics::Snapshot roundtrip{[&metrics](utils::statistics::Writer& writer) {
        for (const auto metric : metrics) {
            DumpMetric(writer, metric);
        }
    }};
    EXPECT_THAT(roundtrip, EqualsNumerically(snapshot));
    EXPECT_THROW(roundtrip.SingleMetric("series"), utils::statistics::MetricQueryError);
    EXPECT_EQ(roundtrip.SingleMetric("series", {{"kind", "a"}}), std::int64_t{2});
    EXPECT_EQ(roundtrip.SingleMetric("series", {{"kind", "b"}}), std::int64_t{3});
}

TEST(Snapshot, MetricViewValidAfterAllMetricsResultDestroyed) {
    const utils::statistics::Snapshot snapshot{[](utils::statistics::Writer& writer) {
        writer["only"] = std::int64_t{42};
    }};

    std::optional<utils::statistics::MetricView> metric;
    {
        auto views = snapshot.AllMetrics();
        auto it = views.begin();
        ASSERT_NE(it, views.end());
        metric = *it;
        ++it;
        EXPECT_EQ(it, views.end());
    }

    ASSERT_TRUE(metric.has_value());
    EXPECT_EQ(metric->path, "only");
    EXPECT_EQ(metric->value, std::int64_t{42});
    EXPECT_TRUE(metric->labels.empty());

    const utils::statistics::Snapshot roundtrip{[&metric](utils::statistics::Writer& writer) {
        DumpMetric(writer, *metric);
    }};
    EXPECT_EQ(roundtrip.SingleMetric("only"), std::int64_t{42});
}

/// [metrics Snapshot filter by iteration]
TEST(Snapshot, FilterByIteratingAllMetrics) {
    const SampleMetrics metrics;
    const utils::statistics::Snapshot source{metrics};

    const utils::statistics::Snapshot filtered{[&source](utils::statistics::Writer& writer) {
        for (const utils::statistics::MetricView metric : source.AllMetrics()) {
            if (metric.path != "foo") {
                continue;
            }
            DumpMetric(writer, metric);
        }
    }};

    EXPECT_EQ(std::ranges::distance(filtered.AllMetrics()), 1);
    EXPECT_EQ(filtered.SingleMetric("foo"), std::int64_t{1});
    EXPECT_EQ(filtered.SingleMetricOptional("bar", {{"kind", "x"}}), std::nullopt);
}
/// [metrics Snapshot filter by iteration]

TEST(Snapshot, DuplicatePathAndLabelsThrow) {
    UEXPECT_THROW(
        utils::statistics::Snapshot::FromJson(formats::json::FromString(R"({
            "requests": [
                {"type": "GAUGE", "value": 1, "labels": {"handler": "/orders"}},
                {"type": "GAUGE", "value": 2, "labels": {"handler": "/orders"}}
            ]
        })")),
        utils::statistics::DuplicateMetricError
    );
    UEXPECT_THROW(
        utils::statistics::Snapshot::FromJson(formats::json::FromString(R"({
            "requests": [
                {
                    "type": "GAUGE",
                    "value": 1,
                    "labels": {"handler": "/orders", "http_code": "200"}
                },
                {
                    "type": "GAUGE",
                    "value": 2,
                    "labels": {"http_code": "200", "handler": "/orders"}
                }
            ]
        })")),
        utils::statistics::DuplicateMetricError
    );
}

TEST(Snapshot, HistogramOutlivesItsWriter) {
    const utils::statistics::Snapshot snapshot{TemporaryQueryMetrics{}};

    for (const auto name : kQueryNames) {
        EXPECT_EQ(snapshot.SingleMetric("timings", {{"query", std::string{name}}}).AsHistogram().GetTotalCount(), 1)
            << name;
    }
}

USERVER_NAMESPACE_END
