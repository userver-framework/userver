#include <utils/statistics/value_builder_helpers.hpp>

#include <userver/formats/json/inline.hpp>
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

USERVER_NAMESPACE_END
