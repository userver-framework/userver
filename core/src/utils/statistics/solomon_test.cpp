#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/utest.hpp>

#include <userver/utils/statistics/metadata.hpp>
#include <userver/utils/statistics/solomon.hpp>
#include <userver/utils/statistics/storage.hpp>

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
  std::sort(
      data.begin(), data.end(),
      [](const formats::json::Value& lhs, const formats::json::Value& rhs) {
        return lhs["labels"]["sensor"].As<std::string>() >
               rhs["labels"]["sensor"].As<std::string>();
      });

  formats::json::ValueBuilder builder;
  for (const auto& metric : data) {
    builder.PushBack(metric);
  }
  return builder.ExtractValue();
}

void TestToMetricsSolomon(const utils::statistics::Storage& statistics,
                          std::string_view raw_expected) {
  const auto expected = Sorted(formats::json::FromString(raw_expected));

  const auto raw_result =
      ToSolomonFormat(statistics, {{"application", "processing"}});
  const auto result_json = formats::json::FromString(raw_result);
  EXPECT_TRUE(result_json.IsObject());
  EXPECT_TRUE(result_json.HasMember("metrics"));

  const auto result = Sorted(result_json["metrics"]);

  EXPECT_EQ(expected, result);
}

}  // namespace

UTEST(MetricsSolomon, Tvm2TicketsCache) {
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

  utils::statistics::Storage statistics_storage;
  auto statistics_holder =
      statistics_storage.RegisterExtender("cache.tvm2-tickets-cache", producer);

  const auto* const expected = R"([
    {"labels": {"sensor": "cache.current-documents-count", "cache_name": "tvm2-tickets-cache"}, "value": 8},
    {"labels": {"sensor": "cache.full.time.time-from-last-update-start-ms", "cache_name": "tvm2-tickets-cache"}, "value": 2521742},
    {"labels": {"sensor": "cache.full.time.time-from-last-successful-start-ms", "cache_name": "tvm2-tickets-cache"}, "value": 2521742},
    {"labels": {"sensor": "cache.full.time.last-update-duration-ms", "cache_name": "tvm2-tickets-cache"}, "value": 58},
    {"labels": {"sensor": "cache.full.documents.read_count", "cache_name": "tvm2-tickets-cache"}, "value": 432},
    {"labels": {"sensor": "cache.full.documents.parse_failures", "cache_name": "tvm2-tickets-cache"}, "value": 0},
    {"labels": {"sensor": "cache.full.update.attempts_count", "cache_name": "tvm2-tickets-cache"}, "value": 56},
    {"labels": {"sensor": "cache.full.update.no_changes_count", "cache_name": "tvm2-tickets-cache"}, "value": 0},
    {"labels": {"sensor": "cache.full.update.failures_count", "cache_name": "tvm2-tickets-cache"}, "value": 0}
  ])";
  TestToMetricsSolomon(statistics_storage, expected);
}

UTEST(MetricsSolomon, SolomonChildrenLabel) {
  auto producer = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonChildrenAreLabelValues(result,
                                                     "child_label_name");
    result["label_value_1"]["ag"]["test"] = 76;
    result["label_value_1"]["ag"]["test1"] = 90;

    result["label_value_2"]["field1"] = 3;
    result["label_value_2"]["field2"] = 6.67;

    utils::statistics::SolomonLabelValue(result["overridden_label_value"],
                                         "overridden_label_name");
    result["overridden_label_value"]["field3"] = 9999;

    return result;
  };

  const auto* const statistics = R"({
    "base_key": {
      "some_key": {
        "$meta": {
          "solomon_children_labels": "child_label_name"
        },
        "label_value_1": {
          "ag": {
            "test": 76,
            "test1": 90
          }
        },
        "label_value_2": {
          "field1": 3,
          "field2": 6.67
        },
        "overridden_label_value": {
          "$meta": {
            "solomon_label": "overridden_label_name"
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

  const auto* const expected = R"([
    {"labels": {"child_label_name": "label_value_1", "sensor": "base_key.some_key.ag.test"}, "value": 76},
    {"labels": {"child_label_name": "label_value_1", "sensor": "base_key.some_key.ag.test1"}, "value": 90},
    {"labels": {"child_label_name": "label_value_2", "sensor": "base_key.some_key.field1"}, "value": 3},
    {"labels": {"child_label_name": "label_value_2", "sensor": "base_key.some_key.field2"}, "value": 6.67},
    {"labels": {"overridden_label_name": "overridden_label_value", "sensor": "base_key.some_key.field3"}, "value": 9999}
  ])";
  TestToMetricsSolomon(statistics_storage, expected);
}

UTEST(MetricsSolomon, SolomonChildrenLabelEscaping) {
  auto producer = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonChildrenAreLabelValues(
        result, R"(child.label.#$/\ _{}'"=name)");
    result[R"(label.value.#$/\ _{}'"1)"]["a.#$/\\ _{}g"]["test"] = 76;
    result[R"(label.value.#$/\ _{}'"1)"]["a.#$/\\ _{}g"]["test1"] = 90;

    result["label.value.#$/\\ _{}2"]["field1"] = 3;
    result["label.value.#$/\\ _{}2"]["field2"] = 6.67;

    utils::statistics::SolomonLabelValue(
        result[R"(overridden.#$/\ _{}'"value)"],
        R"(overridden.#$/\ _{}'"=name)");
    result[R"(overridden.#$/\ _{}'"value)"]["field3"] = 9999;

    return result;
  };

  const auto* const statistics = R"({
    "base_key": {
      "some_key": {
        "$meta": {
          "solomon_children_labels": "child.label.#$/\\ _{}'\"=name"
        },
        "label.value.#$/\\ _{}'\"1": {
          "a.#$/\\ _{}g": {
            "test": 76,
            "test1": 90
          }
        },
        "label.value.#$/\\ _{}2": {
          "field1": 3,
          "field2": 6.67
        },
        "overridden.#$/\\ _{}'\"value": {
          "$meta": {
            "solomon_label": "overridden.#$/\\ _{}'\"=name"
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

  const auto* const expected = R"([
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}'\"1", "sensor": "base_key.some_key.a_#$/\\ _{}g.test"}, "value": 76},
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}'\"1", "sensor": "base_key.some_key.a_#$/\\ _{}g.test1"}, "value": 90},
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}2", "sensor": "base_key.some_key.field1"}, "value": 3},
    {"labels": {"child.label.#$/\\ _{}'\"=name": "label.value.#$/\\ _{}2", "sensor": "base_key.some_key.field2"}, "value": 6.67},
    {"labels": {"overridden.#$/\\ _{}'\"=name": "overridden.#$/\\ _{}'\"value", "sensor": "base_key.some_key.field3"}, "value": 9999}
  ])";
  TestToMetricsSolomon(statistics_storage, expected);
}

UTEST(MetricsSolomon, SimpleStatistics) {
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

  const auto* const expected = R"([
    {"labels": {"sensor": "parent.child1"}, "value": 1},
    {"labels": {"sensor": "parent.child2"}, "value": 2}
  ])";
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder =
        statistics_storage.RegisterExtender("parent", producer1);
    TestToMetricsSolomon(statistics_storage, expected);
  }
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder = statistics_storage.RegisterExtender({}, producer2);
    TestToMetricsSolomon(statistics_storage, expected);
  }
}

UTEST(MetricsSolomon, SimpleParentRenamed) {
  auto producer1 = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonRename(result, "parent");
    result["child"] = 8;
    return result;
  };

  const auto* const statistics = R"({
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

  const auto* const expected = R"([
    {"labels": {"sensor": "parent.child"}, "value": 8}
  ])";
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder =
        statistics_storage.RegisterExtender("parent_renamed", producer1);
    TestToMetricsSolomon(statistics_storage, expected);
  }
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder = statistics_storage.RegisterExtender({}, producer2);
    TestToMetricsSolomon(statistics_storage, expected);
  }
}

UTEST(MetricsSolomon, SimpleParentSkipped) {
  auto producer1 = [](const utils::statistics::StatisticsRequest&) {
    formats::json::ValueBuilder result;
    utils::statistics::SolomonSkip(result);
    result["child"] = 8;
    return result;
  };

  const auto* const statistics = R"({
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

  const auto* const expected = R"([
    {"labels": {"sensor": "child"}, "value": 8}
  ])";

  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder =
        statistics_storage.RegisterExtender("parent_skipped", producer1);
    TestToMetricsSolomon(statistics_storage, expected);
  }
  {
    utils::statistics::Storage statistics_storage;
    auto statistics_holder = statistics_storage.RegisterExtender({}, producer2);
    TestToMetricsSolomon(statistics_storage, expected);
  }
}

}  // namespace utils::statistics::impl

USERVER_NAMESPACE_END
