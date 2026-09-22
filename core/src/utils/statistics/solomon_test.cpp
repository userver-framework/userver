#include <algorithm>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/pretty_format.hpp>
#include <userver/utils/statistics/solomon.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

namespace {

formats::json::Value Sorted(const formats::json::Value& array) {
    EXPECT_TRUE(array.IsArray());

    std::vector<formats::json::Value> data;
    data.reserve(array.GetSize());
    for (const auto& metric : array) {
        data.emplace_back(metric);
    }
    std::ranges::sort(data, [](const formats::json::Value& lhs, const formats::json::Value& rhs) {
        return lhs["labels"]["sensor"].As<std::string>() > rhs["labels"]["sensor"].As<std::string>();
    });

    formats::json::ValueBuilder builder;
    for (const auto& metric : data) {
        builder.PushBack(metric);
    }
    return builder.ExtractValue();
}

void TestToMetricsSolomon(WriterFuncRef writer, std::string_view pretty_expected, std::string_view solomon_expected) {
    EXPECT_EQ(ToPrettyFormat(writer), pretty_expected);

    const auto raw_result = ToSolomonFormat(writer, {{"application", "processing"}});
    const auto result_json = formats::json::FromString(raw_result);
    EXPECT_TRUE(result_json.IsObject());
    EXPECT_TRUE(result_json.HasMember("metrics"));

    EXPECT_EQ(Sorted(formats::json::FromString(solomon_expected)), Sorted(result_json["metrics"]));
}

}  // namespace

UTEST(MetricsSolomon, Tvm2TicketsCache) {
    auto producer = [](Writer& writer) {
        const LabelView cache_name{"cache_name", "tvm2-tickets-cache"};
        writer["cache"]["full"]["update"]["attempts_count"].ValueWithLabels(56, cache_name);
        writer["cache"]["full"]["update"]["no_changes_count"].ValueWithLabels(0, cache_name);
        writer["cache"]["full"]["update"]["failures_count"].ValueWithLabels(0, cache_name);
        writer["cache"]["full"]["documents"]["read_count"].ValueWithLabels(432, cache_name);
        writer["cache"]["full"]["documents"]["parse_failures"].ValueWithLabels(0, cache_name);
        writer["cache"]["full"]["time"]["time-from-last-update-start-ms"].ValueWithLabels(2521742, cache_name);
        writer["cache"]["full"]["time"]["time-from-last-successful-start-ms"].ValueWithLabels(2521742, cache_name);
        writer["cache"]["full"]["time"]["last-update-duration-ms"].ValueWithLabels(58, cache_name);
        writer["cache"]["current-documents-count"].ValueWithLabels(8, cache_name);
    };

    constexpr std::string_view pretty =  //
        "cache.full.update.attempts_count: cache_name=tvm2-tickets-cache\tGAUGE\t56\n"
        "cache.full.update.no_changes_count: cache_name=tvm2-tickets-cache\tGAUGE\t0\n"
        "cache.full.update.failures_count: cache_name=tvm2-tickets-cache\tGAUGE\t0\n"
        "cache.full.documents.read_count: cache_name=tvm2-tickets-cache\tGAUGE\t432\n"
        "cache.full.documents.parse_failures: cache_name=tvm2-tickets-cache\tGAUGE\t0\n"
        "cache.full.time.time-from-last-update-start-ms: cache_name=tvm2-tickets-cache\tGAUGE\t2521742\n"
        "cache.full.time.time-from-last-successful-start-ms: cache_name=tvm2-tickets-cache\tGAUGE\t2521742\n"
        "cache.full.time.last-update-duration-ms: cache_name=tvm2-tickets-cache\tGAUGE\t58\n"
        "cache.current-documents-count: cache_name=tvm2-tickets-cache\tGAUGE\t8\n";

    const auto* const expected = R"([
    {"labels": {"sensor": "cache.current-documents-count", "cache_name": "tvm2-tickets-cache"}, "value": 8, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.time.time-from-last-update-start-ms", "cache_name": "tvm2-tickets-cache"}, "value": 2521742, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.time.time-from-last-successful-start-ms", "cache_name": "tvm2-tickets-cache"}, "value": 2521742, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.time.last-update-duration-ms", "cache_name": "tvm2-tickets-cache"}, "value": 58, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.documents.read_count", "cache_name": "tvm2-tickets-cache"}, "value": 432, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.documents.parse_failures", "cache_name": "tvm2-tickets-cache"}, "value": 0, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.update.attempts_count", "cache_name": "tvm2-tickets-cache"}, "value": 56, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.update.no_changes_count", "cache_name": "tvm2-tickets-cache"}, "value": 0, "type": "IGAUGE"},
    {"labels": {"sensor": "cache.full.update.failures_count", "cache_name": "tvm2-tickets-cache"}, "value": 0, "type": "IGAUGE"}
  ])";
    TestToMetricsSolomon(producer, pretty, expected);
}

UTEST(MetricsSolomon, SolomonChildrenLabel) {
    auto producer = [](Writer& writer) {
        auto some_key = writer["base_key"]["some_key"];
        some_key["ag"]["test"].ValueWithLabels(76, {"child_label_name", "label_value_1"});
        some_key["ag"]["test1"].ValueWithLabels(90, {"child_label_name", "label_value_1"});
        some_key["field1"].ValueWithLabels(3, {"child_label_name", "label_value_2"});
        some_key["field2"].ValueWithLabels(6.67, {"child_label_name", "label_value_2"});
        some_key["field3"].ValueWithLabels(9999, {"overridden_label_name", "overridden_label_value"});
    };

    constexpr std::string_view pretty =  //
        "base_key.some_key.ag.test: child_label_name=label_value_1\tGAUGE\t76\n"
        "base_key.some_key.ag.test1: child_label_name=label_value_1\tGAUGE\t90\n"
        "base_key.some_key.field1: child_label_name=label_value_2\tGAUGE\t3\n"
        "base_key.some_key.field2: child_label_name=label_value_2\tGAUGE\t6.67\n"
        "base_key.some_key.field3: overridden_label_name=overridden_label_value\tGAUGE\t9999\n";

    const auto* const expected = R"([
    {"labels": {"child_label_name": "label_value_1", "sensor": "base_key.some_key.ag.test"}, "value": 76, "type": "IGAUGE"},
    {"labels": {"child_label_name": "label_value_1", "sensor": "base_key.some_key.ag.test1"}, "value": 90, "type": "IGAUGE"},
    {"labels": {"child_label_name": "label_value_2", "sensor": "base_key.some_key.field1"}, "value": 3, "type": "IGAUGE"},
    {"labels": {"child_label_name": "label_value_2", "sensor": "base_key.some_key.field2"}, "value": 6.67},
    {"labels": {"overridden_label_name": "overridden_label_value", "sensor": "base_key.some_key.field3"}, "value": 9999, "type": "IGAUGE"}
  ])";
    TestToMetricsSolomon(producer, pretty, expected);
}

UTEST(MetricsSolomon, SolomonChildrenLabelEscaping) {
    auto producer = [](Writer& writer) {
        auto some_key = writer["base_key"]["some_key"];
        some_key[R"(a.#$/\ _{}g)"]["test"]
            .ValueWithLabels(76, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}'"1)"});
        some_key[R"(a.#$/\ _{}g)"]["test1"]
            .ValueWithLabels(90, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}'"1)"});
        some_key["field1"].ValueWithLabels(3, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}2)"});
        some_key["field2"].ValueWithLabels(6.67, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}2)"});
        some_key["field3"].ValueWithLabels(9999, {R"(overridden.#$/\ _{}'"=name)", R"(overridden.#$/\ _{}'"value)"});
    };

    constexpr std::string_view pretty =  //
        "base_key.some_key.a.#$/\\ _{}g.test: child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}'\"1\tGAUGE\t76\n"
        "base_key.some_key.a.#$/\\ _{}g.test1: child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}'\"1\tGAUGE\t90\n"
        "base_key.some_key.field1: child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}2\tGAUGE\t3\n"
        "base_key.some_key.field2: child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}2\tGAUGE\t6.67\n"
        "base_key.some_key.field3: overridden.#$/\\ _{}'\"=name=overridden.#$/\\ _{}'\"value\tGAUGE\t9999\n";

    const auto* const expected = R"([
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}'\"1", "sensor": "base_key.some_key.a.#$/\\ _{}g.test"}, "value": 76, "type": "IGAUGE"},
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}'\"1", "sensor": "base_key.some_key.a.#$/\\ _{}g.test1"}, "value": 90, "type": "IGAUGE"},
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}2", "sensor": "base_key.some_key.field1"}, "value": 3, "type": "IGAUGE"},
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}2", "sensor": "base_key.some_key.field2"}, "value": 6.67},
    {"labels": {"overridden.#$/\\ _{}'\"=name": "overridden.#$/\\ _{}'\"value", "sensor": "base_key.some_key.field3"}, "value": 9999, "type": "IGAUGE"}
  ])";
    TestToMetricsSolomon(producer, pretty, expected);
}

UTEST(MetricsSolomon, SimpleStatistics) {
    auto producer = [](Writer& writer) {
        writer["parent"]["child1"] = 1;
        writer["parent"]["child2"] = 2;
    };

    constexpr std::string_view pretty =  //
        "parent.child1:\tGAUGE\t1\n"
        "parent.child2:\tGAUGE\t2\n";

    const auto* const expected = R"([
    {"labels": {"sensor": "parent.child1"}, "value": 1, "type": "IGAUGE"},
    {"labels": {"sensor": "parent.child2"}, "value": 2, "type": "IGAUGE"}
  ])";
    TestToMetricsSolomon(producer, pretty, expected);
}

UTEST(MetricsSolomon, SimpleParentRenamed) {
    auto producer = [](Writer& writer) { writer["parent"]["child"] = 8; };

    constexpr std::string_view pretty = "parent.child:\tGAUGE\t8\n";

    const auto* const expected = R"([
    {"labels": {"sensor": "parent.child"}, "value": 8, "type": "IGAUGE"}
  ])";
    TestToMetricsSolomon(producer, pretty, expected);
}

UTEST(MetricsSolomon, SimpleParentSkipped) {
    auto producer = [](Writer& writer) { writer["child"] = 8; };

    constexpr std::string_view pretty = "child:\tGAUGE\t8\n";

    const auto* const expected = R"([
    {"labels": {"sensor": "child"}, "value": 8, "type": "IGAUGE"}
  ])";
    TestToMetricsSolomon(producer, pretty, expected);
}

UTEST(MetricsSolomon, MetricTypes) {
    auto producer = [](Writer& writer) {
        writer["test_metric_types"]["rate-metric"] = Rate{5};
        writer["test_metric_types"]["igauge-metric"] = 6;
        writer["test_metric_types"]["dgauge-metric"] = 6.5;
    };

    constexpr std::string_view pretty =  //
        "test_metric_types.rate-metric:\tRATE\t5\n"
        "test_metric_types.igauge-metric:\tGAUGE\t6\n"
        "test_metric_types.dgauge-metric:\tGAUGE\t6.5\n";

    const auto* const expected = R"([
    {"labels": {"sensor": "test_metric_types.rate-metric"}, "value": 5, "type": "RATE"},
    {"labels": {"sensor": "test_metric_types.igauge-metric"}, "value": 6, "type": "IGAUGE"},
    {"labels": {"sensor": "test_metric_types.dgauge-metric"}, "value": 6.5}
  ])";
    TestToMetricsSolomon(producer, pretty, expected);
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
