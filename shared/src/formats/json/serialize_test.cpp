#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>

#include <formats/common/serialize_test.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct Serialization<formats::json::Value> : public ::testing::Test {
  constexpr static const char* kDoc = "{\"key1\":1,\"key2\":\"val\"}";

  using ValueBuilder = formats::json::ValueBuilder;
  using Value = formats::json::Value;
  using Type = formats::json::Type;

  using ParseException = formats::json::ParseException;
  using TypeMismatchException = formats::json::TypeMismatchException;
  using OutOfBoundsException = formats::json::OutOfBoundsException;
  using MemberMissingException = formats::json::MemberMissingException;
  using BadStreamException = formats::json::BadStreamException;

  constexpr static auto FromString = formats::json::FromString;
  constexpr static auto FromStream = formats::json::FromStream;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, Serialization,
                               formats::json::Value);

namespace {

void TestExceptionMessage(const char* data, const char* expected_msg) {
  using formats::json::FromString;
  using ParseException = formats::json::Value::ParseException;

  try {
    FromString(data);
    FAIL() << "Exception was not thrown on json: " << data;
  } catch (const ParseException& e) {
    EXPECT_TRUE(std::string_view{e.what()}.find(expected_msg) !=
                std::string_view::npos)
        << "JSON " << data
        << " has incorrect line/column error message: " << e.what();
  }
}

}  // namespace

TEST(FormatsJson, ParseErrorLineColumnValidation) {
  TestExceptionMessage(R"~({
}})~",
                       "line 2 column 2");

  TestExceptionMessage(R"~(}{
})~",
                       "line 1 column 1");

  TestExceptionMessage(R"~({
"foo":"bar":"buz"
})~",
                       "line 2 column 12");
}

TEST(FormatsJson, ParseFromBadFile) {
  using formats::json::blocking::FromFile;
  using ParseException = formats::json::Value::ParseException;

  const char* filename = "@ file that / does not exist >< &";
  try {
    FromFile(filename);
    FAIL() << "Exception was not thrown for non existing file";
  } catch (const ParseException& e) {
    EXPECT_TRUE(std::string_view{e.what()}.find(filename) !=
                std::string_view::npos)
        << "No filename in error message: " << e.what();
  }
}

class FmtFormatterParameterized : public testing::TestWithParam<std::string> {};

TEST_P(FmtFormatterParameterized, FormatsJsonFmt) {
  const std::string str = GetParam();
  const auto value = formats::json::FromString(str);
  EXPECT_EQ(fmt::format("{}", value), str);
  EXPECT_THROW(static_cast<void>(fmt::format("{:s}", value)),
               fmt::format_error);
}

INSTANTIATE_TEST_SUITE_P(/* no prefix */, FmtFormatterParameterized,
                         testing::Values(R"({"field":123})", "null", "12345",
                                         "123.45", R"(["abc","def"])"));

TEST(JsonToSortedString, Null) {
  const formats::json::Value example = formats::json::FromString("null");
  ASSERT_EQ(formats::json::ToStableString(example), "null");
}

TEST(JsonToSortedString, Object) {
  const formats::json::Value example =
      formats::json::FromString(R"({"D":{"C":2},"A":1,"B":"sample"})");
  ASSERT_EQ(formats::json::ToStableString(example),
            R"({"A":1,"B":"sample","D":{"C":2}})");
}

TEST(JsonToSortedString, KeysSortedLexicographically) {
  const formats::json::Value example = formats::json::FromString(
      R"({"Sz":1,"Sample":1,"Sam":1,"SampleTest":1,"A":1,"Z":1,"SampleA":1,"SampleZ":1})");
  ASSERT_EQ(
      formats::json::ToStableString(example),
      R"({"A":1,"Sam":1,"Sample":1,"SampleA":1,"SampleTest":1,"SampleZ":1,"Sz":1,"Z":1})");
}

TEST(JsonToSortedString, NestedObjects) {
  const formats::json::Value example =
      formats::json::FromString(R"({"B":{"F":3,"D":1,"E":2},"A":1,"C":3})");
  ASSERT_EQ(formats::json::ToStableString(example),
            R"({"A":1,"B":{"D":1,"E":2,"F":3},"C":3})");
}

TEST(JsonToSortedString, Array) {
  const formats::json::Value example =
      formats::json::FromString(R"({"A":[1,3,2]})");
  ASSERT_EQ(formats::json::ToStableString(example), R"({"A":[1,3,2]})");
}

TEST(JsonToSortedString, ObjectInArray) {
  const formats::json::Value example =
      formats::json::FromString(R"({"A":[1,3,{"D":1,"B":1,"C":1}]})");
  ASSERT_EQ(formats::json::ToStableString(example),
            R"({"A":[1,3,{"B":1,"C":1,"D":1}]})");
}

TEST(JsonToSortedString, ASCII) {
  ASSERT_EQ("\u0041", "A");
  const formats::json::Value escaped =
      formats::json::FromString(R"({"\u0041":1})");
  const formats::json::Value unescaped =
      formats::json::FromString(R"({"A":1})");
  ASSERT_EQ(formats::json::ToStableString(escaped),
            formats::json::ToStableString(unescaped));
}

TEST(JsonToSortedString, NonASCII) {
  ASSERT_EQ("\u5143", "元");
  const formats::json::Value escaped =
      formats::json::FromString(R"({"\u5143":1})");
  const formats::json::Value unescaped =
      formats::json::FromString(R"({"元":1})");
  ASSERT_EQ(formats::json::ToStableString(escaped),
            formats::json::ToStableString(unescaped));
}

USERVER_NAMESPACE_END
