#include <gtest/gtest.h>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/serialize_container.hpp>
#include <formats/json/value.hpp>

#include <formats/common/value_test.hpp>

template <>
struct Parsing<formats::json::Value> : public ::testing::Test {
  constexpr static auto FromString = formats::json::FromString;
  using ParseException = formats::json::Value::ParseException;
};

INSTANTIATE_TYPED_TEST_CASE_P(FormatsJson, Parsing, formats::json::Value);

TEST(FormatsJson, ParsingInvalidRootType) {
  using formats::json::FromString;
  using ParseException = formats::json::Value::ParseException;

  ASSERT_NO_THROW(FromString("{}"));
  ASSERT_TRUE(FromString("{}").IsObject());
  ASSERT_NO_THROW(FromString("[]"));
  ASSERT_TRUE(FromString("[]").IsArray());

  ASSERT_THROW(FromString("null"), ParseException);
  ASSERT_THROW(FromString("true"), ParseException);
  ASSERT_THROW(FromString("0"), ParseException);
  ASSERT_THROW(FromString("1.5"), ParseException);
  ASSERT_THROW(FromString(R"("string")"), ParseException);

  ASSERT_THROW(FromString(R"({"field": 'string'})"), ParseException);
  ASSERT_THROW(FromString("{}{}"), ParseException);
}
