#include <algorithm>

#include <userver/utest/utest.hpp>
#include <userver/utils/statistics/pretty_format.hpp>
#include <userver/utils/statistics/prometheus.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

namespace {

std::string Sorted(const std::string_view raw) {
    auto lines = utils::text::Split(raw, "\n");
    std::ranges::sort(lines);
    return utils::text::Join(lines, "\n");
}

void TestToMetricsPrometheus(
    WriterFuncRef writer,
    std::string_view pretty_expected,
    std::string_view prometheus_expected,
    bool sorted = false
) {
    EXPECT_EQ(ToPrettyFormat(writer), pretty_expected);

    const auto result = ToPrometheusFormat(writer, Request::MakeWithPrefix({}, {{"application", "processing"}}));
    if (sorted) {
        EXPECT_EQ(Sorted(prometheus_expected), Sorted(result));
    } else {
        EXPECT_EQ(prometheus_expected, result);
    }
}

}  // namespace

TEST(MetricsPrometheus, ToPrometheusName) {
    EXPECT_EQ(ToPrometheusName("rss_kb"), "rss_kb");
    EXPECT_EQ(ToPrometheusName("httpclient.timings"), "httpclient_timings");
    EXPECT_EQ(ToPrometheusName("httpclient.reply-statuses"), "httpclient_reply_statuses");
    EXPECT_EQ(ToPrometheusName("httpclient.event-loop-load.1min"), "httpclient_event_loop_load_1min");
    EXPECT_EQ(
        ToPrometheusName("op./v1/cached-value/source-delete.error-parse"),
        "op__v1_cached_value_source_delete_error_parse"
    );
    EXPECT_EQ(ToPrometheusName("processing-ng.queue.status.kEmpty"), "processing_ng_queue_status_kEmpty");
    EXPECT_EQ(ToPrometheusName("1metric"), "_1metric");
    EXPECT_EQ(ToPrometheusName("met:ric"), "met_ric");
}

TEST(MetricsPrometheus, ToPrometheusLabel) {
    EXPECT_EQ(ToPrometheusLabel("percentile"), "percentile");
    EXPECT_EQ(ToPrometheusLabel("postgresql_error"), "postgresql_error");
    EXPECT_EQ(ToPrometheusLabel("http.worker.id"), "http_worker_id");
    EXPECT_TRUE(ToPrometheusLabel("__./__").empty());
    EXPECT_EQ(ToPrometheusLabel("_42"), "_42");
    EXPECT_EQ(ToPrometheusLabel("_???42"), "_42");
    EXPECT_EQ(ToPrometheusName("la:bel"), "la_bel");
}

UTEST(MetricsPrometheus, Tvm2TicketsCache) {
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

    constexpr std::string_view expected = R"(
# TYPE cache_full_update_attempts_count gauge
cache_full_update_attempts_count{application="processing",cache_name="tvm2-tickets-cache"} 56
# TYPE cache_full_update_no_changes_count gauge
cache_full_update_no_changes_count{application="processing",cache_name="tvm2-tickets-cache"} 0
# TYPE cache_full_update_failures_count gauge
cache_full_update_failures_count{application="processing",cache_name="tvm2-tickets-cache"} 0
# TYPE cache_full_documents_read_count gauge
cache_full_documents_read_count{application="processing",cache_name="tvm2-tickets-cache"} 432
# TYPE cache_full_documents_parse_failures gauge
cache_full_documents_parse_failures{application="processing",cache_name="tvm2-tickets-cache"} 0
# TYPE cache_full_time_time_from_last_update_start_ms gauge
cache_full_time_time_from_last_update_start_ms{application="processing",cache_name="tvm2-tickets-cache"} 2521742
# TYPE cache_full_time_time_from_last_successful_start_ms gauge
cache_full_time_time_from_last_successful_start_ms{application="processing",cache_name="tvm2-tickets-cache"} 2521742
# TYPE cache_full_time_last_update_duration_ms gauge
cache_full_time_last_update_duration_ms{application="processing",cache_name="tvm2-tickets-cache"} 58
# TYPE cache_current_documents_count gauge
cache_current_documents_count{application="processing",cache_name="tvm2-tickets-cache"} 8
)";
    TestToMetricsPrometheus(producer, pretty, expected.substr(1));
}

UTEST(Converter, SolomonChildrenLabel) {
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

    constexpr std::string_view expected = R"(
# TYPE base_key_some_key_ag_test gauge
base_key_some_key_ag_test{application="processing",child_label_name="label_value_1"} 76
# TYPE base_key_some_key_ag_test1 gauge
base_key_some_key_ag_test1{application="processing",child_label_name="label_value_1"} 90
# TYPE base_key_some_key_field1 gauge
base_key_some_key_field1{application="processing",child_label_name="label_value_2"} 3
# TYPE base_key_some_key_field2 gauge
base_key_some_key_field2{application="processing",child_label_name="label_value_2"} 6.67
# TYPE base_key_some_key_field3 gauge
base_key_some_key_field3{application="processing",overridden_label_name="overridden_label_value"} 9999
)";
    TestToMetricsPrometheus(producer, pretty, expected.substr(1));
}

UTEST(Converter, SolomonChildrenLabelEscaping) {
    auto producer = [](Writer& writer) {
        auto some_key = writer["base_key"]["some_key"];
        some_key[R"(a.#$/\ _{}g)"]["test"]
            .ValueWithLabels(76, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}'"1)"});
        some_key[R"(a.#$/\ _{}g)"]["test1"]
            .ValueWithLabels(90, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}'"1)"});
        some_key["field1"].ValueWithLabels(3, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}2)"});
        some_key["field2"].ValueWithLabels(6.67, {R"(child.label.#$/\ _{}'"=name)", R"(label.value.#$/\ _{}2)"});
        some_key["field3"]
            .ValueWithLabels(9999, {R"(overridden.label.#$/\ _{}'"=name)", R"(overridden.label.#$/\ _{}'"value)"});
    };

    constexpr std::string_view pretty =  //
        "base_key.some_key.a.#$/\\ _{}g.test: "
        "child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}'\"1\tGAUGE\t76\n"
        "base_key.some_key.a.#$/\\ _{}g.test1: "
        "child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}'\"1\tGAUGE\t90\n"
        "base_key.some_key.field1: child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}2\tGAUGE\t3\n"
        "base_key.some_key.field2: child.label.#$/\\ _{}'\"=name=label.value.#$/\\ _{}2\tGAUGE\t6.67\n"
        "base_key.some_key.field3: "
        "overridden.label.#$/\\ _{}'\"=name=overridden.label.#$/\\ _{}'\"value\tGAUGE\t9999\n";

    constexpr std::string_view expected = R"(
# TYPE base_key_some_key_a_________g_test gauge
base_key_some_key_a_________g_test{application="processing",child_label____________name="label.value.#$/\\ _{}''1"} 76
# TYPE base_key_some_key_a_________g_test1 gauge
base_key_some_key_a_________g_test1{application="processing",child_label____________name="label.value.#$/\\ _{}''1"} 90
# TYPE base_key_some_key_field1 gauge
base_key_some_key_field1{application="processing",child_label____________name="label.value.#$/\\ _{}2"} 3
# TYPE base_key_some_key_field2 gauge
base_key_some_key_field2{application="processing",child_label____________name="label.value.#$/\\ _{}2"} 6.67
# TYPE base_key_some_key_field3 gauge
base_key_some_key_field3{application="processing",overridden_label____________name="overridden.label.#$/\\ _{}''value"} 9999
)";
    TestToMetricsPrometheus(producer, pretty, expected.substr(1));
}

UTEST(MetricsPrometheus, LabelValueBackslashAndNewlineEscaped) {
    auto producer = [](Writer& writer) {
        // A label value with an embedded line feed and a trailing backslash.
        writer["root"]["value"].ValueWithLabels(1, {"label_name", "a\nb\\"});
    };

    constexpr std::string_view pretty = "root.value: label_name=a\nb\\\tGAUGE\t1\n";
    EXPECT_EQ(ToPrettyFormat(producer), pretty);

    const auto result = ToPrometheusFormat(producer, Request::MakeWithPrefix({}, {{"application", "processing"}}));

    // The line feed must be written as `\n` and the trailing backslash doubled.
    // Otherwise the raw backslash escapes the closing quote and the raw line
    // feed starts a new sample line.
    EXPECT_NE(result.find(R"(label_name="a\nb\\")"), std::string::npos) << result;
    // A single gauge metric: only the `# TYPE` line and the sample line, so the
    // escaped value must not add a third line feed.
    EXPECT_EQ(std::ranges::count(result, '\n'), 2) << result;
}

UTEST(MetricsPrometheus, SimpleStatistics) {
    auto producer = [](Writer& writer) {
        writer["parent"]["child1"] = 1;
        writer["parent"]["child2"] = 2;
    };

    constexpr std::string_view pretty =  //
        "parent.child1:\tGAUGE\t1\n"
        "parent.child2:\tGAUGE\t2\n";

    constexpr std::string_view expected = R"(
# TYPE parent_child1 gauge
parent_child1{application="processing"} 1
# TYPE parent_child2 gauge
parent_child2{application="processing"} 2
)";
    TestToMetricsPrometheus(producer, pretty, expected.substr(1));
}

UTEST(MetricsPrometheus, SimpleParentRenamed) {
    auto producer = [](Writer& writer) { writer["parent"]["child"] = 8; };

    constexpr std::string_view pretty = "parent.child:\tGAUGE\t8\n";

    constexpr std::string_view expected = R"(
# TYPE parent_child gauge
parent_child{application="processing"} 8
)";
    TestToMetricsPrometheus(producer, pretty, expected.substr(1));
}

UTEST(MetricsPrometheus, SimpleParentSkipped) {
    auto producer = [](Writer& writer) { writer["child"] = 8; };

    constexpr std::string_view pretty = "child:\tGAUGE\t8\n";

    constexpr std::string_view expected = R"(
# TYPE child gauge
child{application="processing"} 8
)";
    TestToMetricsPrometheus(producer, pretty, expected.substr(1));
}

UTEST(MetricsPrometheus, MetricTypes) {
    auto producer = [](Writer& writer) {
        writer["test_metric_types"]["rate-metric"] = Rate{5};
        writer["test_metric_types"]["igauge-metric"] = 6;
        writer["test_metric_types"]["dgauge-metric"] = 6.5;
    };

    constexpr std::string_view pretty =  //
        "test_metric_types.rate-metric:\tRATE\t5\n"
        "test_metric_types.igauge-metric:\tGAUGE\t6\n"
        "test_metric_types.dgauge-metric:\tGAUGE\t6.5\n";

    constexpr std::string_view expected = R"(
# TYPE test_metric_types_rate_metric counter
test_metric_types_rate_metric{application="processing"} 5
# TYPE test_metric_types_igauge_metric gauge
test_metric_types_igauge_metric{application="processing"} 6
# TYPE test_metric_types_dgauge_metric gauge
test_metric_types_dgauge_metric{application="processing"} 6.5
)";
    TestToMetricsPrometheus(producer, pretty, expected.substr(1));
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
