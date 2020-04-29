#include <gtest/gtest.h>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/serialize_container.hpp>
#include <formats/json/value.hpp>
#include <formats/json/value_builder.hpp>

#include <formats/common/value_test.hpp>

template <>
struct Parsing<formats::json::Value> : public ::testing::Test {
  constexpr static auto FromString = formats::json::FromString;
  using ParseException = formats::json::Value::ParseException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, Parsing, formats::json::Value);

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

void TestLargeDoubleValueAsInt64(const std::string& json_str,
                                 int64_t expected_value, bool expect_ok) {
  auto json = formats::json::FromString(json_str);
  int64_t parsed_value = 0;
  if (expect_ok) {
    ASSERT_NO_THROW(parsed_value = json["value"].As<int64_t>())
        << "json: " << json_str;
    EXPECT_EQ(parsed_value, expected_value)
        << "json: " << json_str
        << ", parsed double: " << json["value"].As<double>();
  } else {
    ASSERT_THROW(parsed_value = json["value"].As<int64_t>(),
                 formats::json::TypeMismatchException)
        << "json: " << json_str;
  }
}

class TestIncorrectValueException : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

void CheckExactValues(int bits) {
  int64_t start = (1L << bits);
  for (int add = -20; add <= 0; ++add) {
    int64_t value = start + add;
    std::string json_str = R"({"value": )" + std::to_string(value) + ".0}";
    auto json = formats::json::FromString(json_str);
    double dval = json["value"].As<double>();
    int64_t ival = static_cast<int64_t>(dval);
    if (ival != value) throw TestIncorrectValueException("test");
  }
}

TEST(FormatsJson, LargeDoubleValueAsInt64) {
  const int kMaxCorrectBits = 53;

  for (int bits = kMaxCorrectBits; bits >= kMaxCorrectBits - 5; --bits) {
    int64_t start = (1L << bits);
    int max_add = bits == kMaxCorrectBits ? -1 : 20;
    for (int add = max_add; add >= -20; --add) {
      int64_t value = start + add;
      std::string json_str = R"({"value": )" + std::to_string(value) + ".0}";
      TestLargeDoubleValueAsInt64(json_str, value, true);
      json_str = R"({"value": )" + std::to_string(-value) + ".0}";
      TestLargeDoubleValueAsInt64(json_str, -value, true);
    }
  }

  ASSERT_THROW(CheckExactValues(kMaxCorrectBits + 1),
               TestIncorrectValueException);

  // 2 ** 53 == 9007199254740992
  TestLargeDoubleValueAsInt64(R"({"value": 9007199254740992.0})",
                              9007199254740992, false);
  TestLargeDoubleValueAsInt64(R"({"value": 9007199254740993.0})",
                              9007199254740993, false);
  TestLargeDoubleValueAsInt64(R"({"value": -9007199254740992.0})",
                              -9007199254740992, false);
  TestLargeDoubleValueAsInt64(R"({"value": -9007199254740993.0})",
                              -9007199254740993, false);
}

TEST(FormatsJson, ParseNanInf) {
  using formats::json::FromString;
  using ParseException = formats::json::Value::ParseException;

  ASSERT_THROW(FromString(R"({"field": NaN})"), ParseException);
  ASSERT_THROW(FromString(R"({"field": Inf})"), ParseException);
  ASSERT_THROW(FromString(R"({"field": -Inf})"), ParseException);
}
