#include <utils/statistics/value_builder_helpers.hpp>

#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

using utils::statistics::SetSubField;

TEST(SetSubField, Single) {
  formats::json::ValueBuilder value;
  value["1"] = 1;

  formats::json::ValueBuilder result;
  SetSubField(result, {"obj"}, std::move(value));

  formats::json::ValueBuilder cmp;
  cmp["obj"]["1"] = 1;

  EXPECT_EQ(cmp.ExtractValue(), result.ExtractValue());
}

TEST(SetSubField, Multiple) {
  formats::json::ValueBuilder value;
  value["1"] = 0;

  formats::json::ValueBuilder result;
  SetSubField(result, {"a", "bc"}, std::move(value));

  formats::json::ValueBuilder cmp;
  cmp["a"]["bc"]["1"] = 0;

  EXPECT_EQ(cmp.ExtractValue(), result.ExtractValue());
}

TEST(SetSubField, Empty) {
  formats::json::ValueBuilder value;
  value["1"] = 0;

  formats::json::ValueBuilder result;
  SetSubField(result, {}, std::move(value));

  formats::json::ValueBuilder cmp;
  cmp["1"] = 0;

  EXPECT_EQ(cmp.ExtractValue(), result.ExtractValue());
}

TEST(SetSubField, Merge) {
  formats::json::ValueBuilder result;
  result["a"]["b"] = 1;

  SetSubField(result, {"a"}, formats::json::MakeObject("c", 2));

  formats::json::ValueBuilder expected;
  expected["a"]["b"] = 1;
  expected["a"]["c"] = 2;

  EXPECT_EQ(result.ExtractValue(), expected.ExtractValue());
}

TEST(SetSubField, MergeRecursive) {
  formats::json::ValueBuilder result;
  result["a"]["b"]["c"] = 1;

  formats::json::ValueBuilder patch;
  patch["b"]["d"] = 2;

  SetSubField(result, {"a"}, std::move(patch));

  formats::json::ValueBuilder expected;
  expected["a"]["b"]["c"] = 1;
  expected["a"]["b"]["d"] = 2;

  EXPECT_EQ(result.ExtractValue(), expected.ExtractValue());
}

TEST(SetSubField, EmptyObject) {
  formats::json::ValueBuilder result;

  SetSubField(result, {"a"},
              formats::json::ValueBuilder(formats::json::Type::kObject));

  const auto result_object = result.ExtractValue();
  EXPECT_TRUE(result_object["a"].IsObject());
  EXPECT_TRUE(result_object["a"].IsEmpty());
}

TEST(AreAllMetricsNumbers, DISABLED_Null) {
  const auto json = formats::json::FromString(R"({
    "my_value" : {
      "field1" : 1.34,
      "field2" : null
    }
  })");

  EXPECT_EQ(utils::statistics::FindNonNumberMetricPath(json),
            "my_value.field2");
}

TEST(AreAllMetricsNumbers, Bool) {
  const auto json = formats::json::FromString(R"({
    "my_value" : {
      "field1" : 1.34,
      "field2" : true
    }
  })");

  EXPECT_EQ(utils::statistics::FindNonNumberMetricPath(json),
            "my_value.field2");
}

TEST(AreAllMetricsNumbers, Array) {
  const auto json = formats::json::FromString(R"({
    "my_value" : {
      "arr" : [
        {
          "field1" : 1.23
        },
        {
          "field2" : -5424
        }
      ]
    }
  })");

  EXPECT_EQ(utils::statistics::FindNonNumberMetricPath(json), "my_value.arr");
}

TEST(AreAllMetricsNumbers, Numbers) {
  const auto json = formats::json::FromString(R"({
    "my_value" : {
      "object" :
        {
          "field1" : 1.23,
          "field2" : -5424
        },
      "field" : 1.0995116e+12
    }
  })");

  EXPECT_FALSE(utils::statistics::FindNonNumberMetricPath(json));
}

TEST(AreAllMetricsNumbers, Meta) {
  const auto json = formats::json::FromString(R"({
    "my_value" : {
      "$meta" :
        {
          "field1" : null,
          "field2" : false
        },
      "field" : 1.0995116e+12
    }
  })");

  EXPECT_FALSE(utils::statistics::FindNonNumberMetricPath(json));
}

USERVER_NAMESPACE_END
