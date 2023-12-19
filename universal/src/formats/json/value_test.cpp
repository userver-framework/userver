#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/serialize_container.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/literals.hpp>

#include <formats/common/value_test.hpp>

USERVER_NAMESPACE_BEGIN

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

  ASSERT_NO_THROW(FromString("null"));
  ASSERT_NO_THROW(FromString("true"));
  ASSERT_NO_THROW(FromString("false"));
  ASSERT_NO_THROW(FromString("0"));
  ASSERT_NO_THROW(FromString("1.5"));
  ASSERT_NO_THROW(FromString("-1.2e-0123"));
  ASSERT_NO_THROW(FromString("-1.2E34"));
  ASSERT_NO_THROW(FromString("1.2E+34"));
  ASSERT_NO_THROW(FromString(R"("string")"));
  ASSERT_NO_THROW(FromString(R"("")"));

  ASSERT_THROW(FromString("NULL"), ParseException);
  ASSERT_THROW(FromString("True"), ParseException);
  ASSERT_THROW(FromString("00"), ParseException);
  ASSERT_THROW(FromString(""), ParseException);
  ASSERT_THROW(FromString("inf"), ParseException);
  ASSERT_THROW(FromString("#INF"), ParseException);
  ASSERT_THROW(FromString("nan"), ParseException);
  ASSERT_THROW(FromString("NaN"), ParseException);

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
    auto dval = json["value"].As<double>();
    auto ival = static_cast<int64_t>(dval);
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

TEST(FormatsJson, NulString) {
  std::string i_contain_nuls = "test";
  i_contain_nuls += '\x00';
  i_contain_nuls += "test";

  auto s = formats::json::ValueBuilder(i_contain_nuls)
               .ExtractValue()
               .As<std::string>();
  ASSERT_EQ(i_contain_nuls, s);
}

TEST(FormatsJson, NullAsDefaulted) {
  using formats::json::FromString;
  auto json = FromString(R"~({"nulled": null})~");

  EXPECT_EQ(json["nulled"].As<int>({}), 0);
  EXPECT_EQ(json["nulled"].As<std::vector<int>>({}), std::vector<int>{});

  EXPECT_EQ(json["nulled"].As<int>(42), 42);

  std::vector<int> value{4, 2};
  EXPECT_EQ(json["nulled"].As<std::vector<int>>(value), value);
}

TEST(FormatsJson, ExampleUsage) {
  /// [Sample formats::json::Value usage]
  // #include <userver/formats/json.hpp>

  formats::json::Value json = formats::json::FromString(R"({
    "key1": 1,
    "key2": {"key3":"val"}
  })");

  const auto key1 = json["key1"].As<int>();
  ASSERT_EQ(key1, 1);

  const auto key3 = json["key2"]["key3"].As<std::string>();
  ASSERT_EQ(key3, "val");
  /// [Sample formats::json::Value usage]
}

/// [Sample formats::json::Value::As<T>() usage]
namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};
//  The function must be declared in the namespace of your type
MyKeyValue Parse(const formats::json::Value& json,
                 formats::parse::To<MyKeyValue>) {
  return MyKeyValue{
      json["field1"].As<std::string>(""),
      json["field2"].As<int>(1),  // return `1` if "field2" is missing
  };
}

TEST(FormatsJson, ExampleUsageMyStruct) {
  formats::json::Value json = formats::json::FromString(R"({
    "my_value": {
        "field1": "one",
        "field2": 1
    }
  })");
  auto data = json["my_value"].As<MyKeyValue>();
  EXPECT_EQ(data.field1, "one");
  EXPECT_EQ(data.field2, 1);
}
}  // namespace my_namespace
/// [Sample formats::json::Value::As<T>() usage]

TEST(FormatsJson, UserDefinedLiterals) {
  using ValueBuilder = formats::json::ValueBuilder;
  ValueBuilder builder{formats::common::Type::kObject};
  builder["test"] = 3;
  EXPECT_EQ(builder.ExtractValue(),
            R"json(
    {"test" : 3}
    )json"_json);
}

USERVER_NAMESPACE_END
