#include <userver/utest/utest.hpp>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

#include <userver/formats/json/serialize.hpp>
#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/storage.hpp>

#include <userver/utils/statistics/prometheus.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics::impl {

namespace {

std::string Sorted(const std::string_view raw) {
  std::vector<std::string> lines;
  boost::split(lines, raw, boost::is_any_of("\n"));
  std::sort(lines.begin(), lines.end());
  return boost::algorithm::join(lines, "\n");
}

void TestToMetricsPrometheus(const utils::statistics::Storage& statistics,
                             const std::string_view expected,
                             const bool sorted = false) {
  const auto result = ToPrometheusFormat(
      statistics, utils::statistics::StatisticsRequest::MakeWithPrefix(
                      {}, {{"application", "processing"}}));
  if (sorted) {
    EXPECT_EQ(Sorted(expected), Sorted(result));
  } else {
    EXPECT_EQ(expected, result);
  }
}

}  // namespace

TEST(MetricsPrometheus, ToPrometheusName) {
  EXPECT_EQ(ToPrometheusName("rss_kb"), "rss_kb");
  EXPECT_EQ(ToPrometheusName("httpclient.timings"), "httpclient_timings");
  EXPECT_EQ(ToPrometheusName("httpclient.reply-statuses"),
            "httpclient_reply_statuses");
  EXPECT_EQ(ToPrometheusName("httpclient.event-loop-load.1min"),
            "httpclient_event_loop_load_1min");
  EXPECT_EQ(ToPrometheusName("op./v1/cached-value/source-delete.error-parse"),
            "op__v1_cached_value_source_delete_error_parse");
  EXPECT_EQ(ToPrometheusName("processing-ng.queue.status.kEmpty"),
            "processing_ng_queue_status_kEmpty");
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
  auto producer = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonLabelValue(result, "cache_name");
    result["full"]["update"]["attempts_count"] = 56;
    result["full"]["update"]["no_changes_count"] = 0;
    result["full"]["update"]["failures_count"] = 0;

    result["full"]["documents"]["read_count"] = 432;
    result["full"]["documents"]["parse_failures"] = 0;

    result["full"]["time"]["time-from-last-update-start-ms"] = 2521742;
    result["full"]["time"]["time-from-last-successful-start-ms"] = 2521742;
    result["full"]["time"]["last-update-duration-ms"] = 58;

    result["current-documents-count"] = 8;

    return result;
  };

  utils::statistics::Storage statistics_storage;
  auto statistics_holder =
      statistics_storage.RegisterExtender("cache.tvm2-tickets-cache", producer);

  const auto* const statistics = R"({
    "cache": {
      "tvm2-tickets-cache": {
        "$meta": {
          "solomon_label": "cache_name"
        },
        "full": {
          "update": {
            "attempts_count": 56,
            "no_changes_count": 0,
            "failures_count": 0
          },
          "documents": {
            "read_count": 432,
            "parse_failures": 0
          },
          "time": {
            "time-from-last-update-start-ms": 2521742,
            "time-from-last-successful-start-ms": 2521742,
            "last-update-duration-ms": 58
          }
        },
        "current-documents-count": 8
      }
    }
  })";

  EXPECT_EQ(
      producer({}).ExtractValue(),
      formats::json::FromString(statistics)["cache"]["tvm2-tickets-cache"]);

  const auto* const expected =
      "# TYPE cache_current_documents_count gauge\n"
      "cache_current_documents_count{application=\"processing\",cache_name="
      "\"tvm2-tickets-cache\"} 8\n"
      "# TYPE cache_full_time_time_from_last_update_start_ms gauge\n"
      "cache_full_time_time_from_last_update_start_ms{application="
      "\"processing\",cache_name=\"tvm2-tickets-cache\"} 2521742\n"
      "# TYPE cache_full_time_time_from_last_successful_start_ms gauge\n"
      "cache_full_time_time_from_last_successful_start_ms{application="
      "\"processing\",cache_name=\"tvm2-tickets-cache\"} 2521742\n"
      "# TYPE cache_full_time_last_update_duration_ms gauge\n"
      "cache_full_time_last_update_duration_ms{application=\"processing\","
      "cache_name=\"tvm2-tickets-cache\"} 58\n"
      "# TYPE cache_full_documents_read_count gauge\n"
      "cache_full_documents_read_count{application=\"processing\",cache_name="
      "\"tvm2-tickets-cache\"} 432\n"
      "# TYPE cache_full_documents_parse_failures gauge\n"
      "cache_full_documents_parse_failures{application=\"processing\",cache_"
      "name=\"tvm2-tickets-cache\"} 0\n"
      "# TYPE cache_full_update_attempts_count gauge\n"
      "cache_full_update_attempts_count{application=\"processing\",cache_name="
      "\"tvm2-tickets-cache\"} 56\n"
      "# TYPE cache_full_update_no_changes_count gauge\n"
      "cache_full_update_no_changes_count{application=\"processing\",cache_"
      "name=\"tvm2-tickets-cache\"} 0\n"
      "# TYPE cache_full_update_failures_count gauge\n"
      "cache_full_update_failures_count{application=\"processing\",cache_name="
      "\"tvm2-tickets-cache\"} 0\n";
  TestToMetricsPrometheus(statistics_storage, expected);
}

UTEST(Converter, SolomonChildrenLabel) {
  auto producer = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonChildrenAreLabelValues(result,
                                                     "child_lable_name");
    result["lable_value_1"]["ag"]["test"] = 76;
    result["lable_value_1"]["ag"]["test1"] = 90;

    result["lable_value_2"]["field1"] = 3;
    result["lable_value_2"]["field2"] = 6.67;

    utils::statistics::SolomonLabelValue(result["overriden_lable_value"],
                                         "overriden_lable_name");
    result["overriden_lable_value"]["field3"] = 9999;

    return result;
  };

  const auto* const statistics = R"({
    "base_key": {
      "some_key": {
        "$meta": {
          "solomon_children_labels": "child_lable_name"
        },
        "lable_value_1": {
          "ag": {
            "test": 76,
            "test1": 90
          }
        },
        "lable_value_2": {
          "field1": 3,
          "field2": 6.67
        },
        "overriden_lable_value": {
          "$meta": {
            "solomon_label": "overriden_lable_name"
          },
          "field3": 9999
        }
      }
    }
  })";

  EXPECT_EQ(producer({}).ExtractValue(),
            formats::json::FromString(statistics)["base_key"]["some_key"]);

  utils::statistics::Storage statistics_storage;
  auto statistics_holder =
      statistics_storage.RegisterExtender("base_key.some_key", producer);

  const auto* const expected =
      "# TYPE base_key_some_key_ag_test gauge\n"
      "base_key_some_key_ag_test{application=\"processing\",child_lable_name="
      "\"lable_value_1\"} 76\n"
      "# TYPE base_key_some_key_ag_test1 gauge\n"
      "base_key_some_key_ag_test1{application=\"processing\",child_lable_name="
      "\"lable_value_1\"} 90\n"
      "# TYPE base_key_some_key_field1 gauge\n"
      "base_key_some_key_field1{application=\"processing\",child_lable_name="
      "\"lable_value_2\"} 3\n"
      "# TYPE base_key_some_key_field2 gauge\n"
      "base_key_some_key_field2{application=\"processing\",child_lable_name="
      "\"lable_value_2\"} 6.67\n"
      "# TYPE base_key_some_key_field3 gauge\n"
      "base_key_some_key_field3{application=\"processing\",overriden_lable_"
      "name=\"overriden_lable_value\"} 9999\n";
  TestToMetricsPrometheus(statistics_storage, expected, true);
}

UTEST(Converter, SolomonChildrenLabelEscaping) {
  auto producer = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonChildrenAreLabelValues(
        result, R"~(child.lable.#$/\ _{}'"=name)~");
    result[R"~(lable.value.#$/\ _{}'"1)~"][R"~(a.#$/\ _{}g)~"]["test"] = 76;
    result[R"~(lable.value.#$/\ _{}'"1)~"][R"~(a.#$/\ _{}g)~"]["test1"] = 90;

    result[R"~(lable.value.#$/\ _{}2)~"]["field1"] = 3;
    result[R"~(lable.value.#$/\ _{}2)~"]["field2"] = 6.67;

    utils::statistics::SolomonLabelValue(
        result[R"~(overriden.lable.#$/\ _{}'"value)~"],
        R"~(overriden.lable.#$/\ _{}'"=name)~");
    result[R"~(overriden.lable.#$/\ _{}'"value)~"]["field3"] = 9999;

    return result;
  };

  const auto* const statistics = R"({
    "base_key": {
      "some_key": {
        "$meta": {
          "solomon_children_labels": "child.lable.#$/\\ _{}'\"=name"
        },
        "lable.value.#$/\\ _{}'\"1": {
          "a.#$/\\ _{}g": {
            "test": 76,
            "test1": 90
          }
        },
        "lable.value.#$/\\ _{}2": {
          "field1": 3,
          "field2": 6.67
        },
        "overriden.lable.#$/\\ _{}'\"value": {
          "$meta": {
            "solomon_label": "overriden.lable.#$/\\ _{}'\"=name"
          },
          "field3": 9999
        }
      }
    }
  })";

  EXPECT_EQ(producer({}).ExtractValue(),
            formats::json::FromString(statistics)["base_key"]["some_key"]);

  utils::statistics::Storage statistics_storage;
  auto statistics_holder =
      statistics_storage.RegisterExtender("base_key.some_key", producer);

  const auto* const expected =
      R"(# TYPE base_key_some_key_a_________g_test gauge
base_key_some_key_a_________g_test{application="processing",child_lable____________name="lable.value.#$/\ _{}''1"} 76
# TYPE base_key_some_key_a_________g_test1 gauge
base_key_some_key_a_________g_test1{application="processing",child_lable____________name="lable.value.#$/\ _{}''1"} 90
# TYPE base_key_some_key_field1 gauge
base_key_some_key_field1{application="processing",child_lable____________name="lable.value.#$/\ _{}2"} 3
# TYPE base_key_some_key_field2 gauge
base_key_some_key_field2{application="processing",child_lable____________name="lable.value.#$/\ _{}2"} 6.67
# TYPE base_key_some_key_field3 gauge
base_key_some_key_field3{application="processing",overriden_lable____________name="overriden.lable.#$/\ _{}''value"} 9999
)";
  TestToMetricsPrometheus(statistics_storage, expected, true);
}

UTEST(MetricsPrometheus, SimpleStatistics) {
  auto producer1 = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    result["child1"] = 1;
    result["child2"] = 2;
    return result;
  };

  const auto* const statistics = R"({
    "parent": {
      "child1": 1,
      "child2": 2
    }
  })";

  EXPECT_EQ(producer1({}).ExtractValue(),
            formats::json::FromString(statistics)["parent"]);

  auto producer2 = [producer1](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    result["parent"] = producer1({});
    return result;
  };
  EXPECT_EQ(producer2({}).ExtractValue(),
            formats::json::FromString(statistics));

  const auto* expected =
      "# TYPE parent_child1 gauge\n"
      "parent_child1{application=\"processing\"} 1\n"
      "# TYPE parent_child2 gauge\n"
      "parent_child2{application=\"processing\"} 2\n";
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder =
        statistics_storage.RegisterExtender("parent", producer1);
    TestToMetricsPrometheus(statistics_storage, expected);
  }
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder = statistics_storage.RegisterExtender({}, producer2);
    TestToMetricsPrometheus(statistics_storage, expected);
  }
}

UTEST(MetricsPrometheus, SimpleParentRenamed) {
  auto producer1 = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonRename(result, "parent");
    result["child"] = 8;
    return result;
  };

  const auto* statistics = R"({
    "parent_renamed": {
      "$meta": {
        "solomon_rename": "parent"
      },
      "child": 8
    }
  })";

  EXPECT_EQ(producer1({}).ExtractValue(),
            formats::json::FromString(statistics)["parent_renamed"]);

  auto producer2 = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonRename(result["parent_renamed"], "parent");
    result["parent_renamed"]["child"] = 8;
    return result;
  };

  EXPECT_EQ(producer2({}).ExtractValue(),
            formats::json::FromString(statistics));

  const auto* expected =
      "# TYPE parent_child gauge\n"
      "parent_child{application=\"processing\"} 8\n";
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder =
        statistics_storage.RegisterExtender("parent", producer1);
    TestToMetricsPrometheus(statistics_storage, expected);
  }
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder = statistics_storage.RegisterExtender({}, producer2);
    TestToMetricsPrometheus(statistics_storage, expected);
  }
}

UTEST(MetricsPrometheus, SimpleParentSkipped) {
  auto producer1 = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonSkip(result);
    result["child"] = 8;
    return result;
  };

  const auto* statistics = R"({
    "parent_skipped": {
      "$meta": {
        "solomon_skip": true
      },
      "child": 8
    }
  })";

  EXPECT_EQ(producer1({}).ExtractValue(),
            formats::json::FromString(statistics)["parent_skipped"]);

  auto producer2 = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonSkip(result["parent_skipped"]);
    result["parent_skipped"]["child"] = 8;
    return result;
  };

  EXPECT_EQ(producer2({}).ExtractValue(),
            formats::json::FromString(statistics));

  const auto* expected =
      "# TYPE child gauge\n"
      "child{application=\"processing\"} 8\n";
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder =
        statistics_storage.RegisterExtender("parent_skipped", producer1);
    TestToMetricsPrometheus(statistics_storage, expected);
  }
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder = statistics_storage.RegisterExtender({}, producer2);
    TestToMetricsPrometheus(statistics_storage, expected);
  }
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
