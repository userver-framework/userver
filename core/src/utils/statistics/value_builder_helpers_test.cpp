#include <userver/utest/utest.hpp>
#include <utils/statistics/value_builder_helpers.hpp>

USERVER_NAMESPACE_BEGIN

TEST(SetSubField, single) {
  formats::json::ValueBuilder value;
  value["1"] = 1;

  formats::json::ValueBuilder result;
  utils::statistics::SetSubField(result, "obj", std::move(value));

  formats::json::ValueBuilder cmp;
  cmp["obj"]["1"] = 1;

  EXPECT_EQ(cmp.ExtractValue(), result.ExtractValue());
}

TEST(SetSubField, multiple) {
  formats::json::ValueBuilder value;
  value["1"] = 0;

  formats::json::ValueBuilder result;
  utils::statistics::SetSubField(result, "a.bc", std::move(value));

  formats::json::ValueBuilder cmp;
  cmp["a"]["bc"]["1"] = 0;

  EXPECT_EQ(cmp.ExtractValue(), result.ExtractValue());
}

TEST(SetSubField, empty) {
  formats::json::ValueBuilder value;
  value["1"] = 0;

  formats::json::ValueBuilder result;
  utils::statistics::SetSubField(result, "", std::move(value));

  formats::json::ValueBuilder cmp;
  cmp["1"] = 0;

  EXPECT_EQ(cmp.ExtractValue(), result.ExtractValue());
}

USERVER_NAMESPACE_END
