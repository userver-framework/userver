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

USERVER_NAMESPACE_END
