#include <gtest/gtest.h>

#include <array>
#include <cstring>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/serialize_duration.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/death_tests.hpp>

USERVER_NAMESPACE_BEGIN

using formats::json::FromString;
using formats::json::StringBuilder;
using formats::json::ValueBuilder;

namespace formats::serialize::impl {

template <class Value>
constexpr inline bool
    kIsSerializeAllowedInWriteToStream<Value, formats::json::StringBuilder> =
        false;

}  // namespace formats::serialize::impl

// Sample from codegen, to make sure that everything works fine
namespace testing {

// Only for testing. In codegen it will have some fields
class Response200EchoobjectwithmappingA final {
 public:
};

formats::json::Value Serialize(const Response200EchoobjectwithmappingA& value,
                               formats::serialize::To<formats::json::Value>);

void WriteToStream(const Response200EchoobjectwithmappingA& value,
                   formats::json::StringBuilder& sw, bool hide_brackets = false,
                   const char* hide_field_name = nullptr);

struct Response200 {
  ::std::string object_no_mapping_type;
  ::std::vector<testing::Response200EchoobjectwithmappingA>
      echoobjectwithmapping;
};

void WriteToStream(const Response200& value, formats::json::StringBuilder& sw,
                   bool hide_brackets = false,
                   const char* hide_field_name = nullptr);

void WriteToStream(
    [[maybe_unused]] const Response200EchoobjectwithmappingA& value,
    formats::json::StringBuilder& sw, bool /*hide_brackets*/,
    [[maybe_unused]] const char* /*hide_field_name*/) {
  formats::json::StringBuilder::ObjectGuard guard{sw};
}

void WriteToStream([[maybe_unused]] const Response200& value,
                   formats::json::StringBuilder& sw, bool hide_brackets,
                   [[maybe_unused]] const char* hide_field_name) {
  std::optional<formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (!hide_field_name ||
      std::strcmp(hide_field_name, "object_no_mapping_type") != 0) {
    sw.Key("object_no_mapping_type");

    WriteToStream(value.object_no_mapping_type, sw);
  }

  if (!hide_field_name ||
      std::strcmp(hide_field_name, "echoObjectWithMapping") != 0) {
    sw.Key("echoObjectWithMapping");

    WriteToStream(value.echoobjectwithmapping, sw);
  }
}

}  // namespace testing

TEST(JsonStringBuilder, Null) {
  StringBuilder sw;
  sw.WriteNull();

  EXPECT_EQ("null", sw.GetString());
}

TEST(JsonStringBuilder, BoolTrue) {
  StringBuilder sw;
  sw.WriteBool(true);

  EXPECT_EQ("true", sw.GetString());
}

TEST(JsonStringBuilder, BoolFalse) {
  StringBuilder sw;
  sw.WriteBool(false);

  EXPECT_EQ("false", sw.GetString());
}

TEST(JsonStringBuilder, Int64) {
  StringBuilder sw;
  sw.WriteInt64(-123);

  EXPECT_EQ("-123", sw.GetString());
}

TEST(JsonStringBuilder, UInt64) {
  StringBuilder sw;
  sw.WriteUInt64(123U);

  EXPECT_EQ("123", sw.GetString());
}

TEST(JsonStringBuilder, Double) {
  StringBuilder sw;
  sw.WriteDouble(12.3);

  EXPECT_EQ("12.3", sw.GetString());
}

TEST(JsonStringBuilder, Object) {
  StringBuilder sw;
  {
    StringBuilder::ObjectGuard guard(sw);
    sw.Key("123");
    sw.WriteInt64(42);
  }

  EXPECT_EQ("{\"123\":42}", sw.GetString());
}

TEST(JsonStringBuilder, Array) {
  StringBuilder sw;
  {
    StringBuilder::ArrayGuard guard(sw);
    sw.WriteString("123");
    sw.WriteBool(true);
  }

  EXPECT_EQ("[\"123\",true]", sw.GetString());
}

TEST(JsonStringBuilder, RawValue) {
  StringBuilder sw;
  {
    StringBuilder::ArrayGuard guard(sw);
    sw.WriteRawString("{1:2}");
  }

  EXPECT_EQ("[{1:2}]", sw.GetString());
}

TEST(JsonStringBuilder, TestResponse) {
  testing::Response200 v{};
  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_FALSE(sw.GetString().empty());
}

TEST(JsonStringBuilder, Value) {
  ValueBuilder builder;
  builder["k"] = "a";
  builder["k2"] = "b";
  auto value = builder.ExtractValue();

  StringBuilder sw;
  WriteToStream(value, sw);
  auto str = sw.GetString();

  EXPECT_EQ(value, FromString(str));
}

TEST(JsonStringBuilder, String) {
  StringBuilder sw;
  WriteToStream(std::string("some string"), sw);
  EXPECT_EQ(sw.GetString(), "\"some string\"");
}

TEST(JsonStringBuilder, VectorBool) {
  std::vector<bool> v = {true, false};
  StringBuilder sw;
  WriteToStream(v, sw);
  EXPECT_EQ(sw.GetString(), "[true,false]");
}

TEST(JsonStringBuilder, CharP) {
  StringBuilder sw;
  WriteToStream("some string", sw);
  EXPECT_EQ(sw.GetString(), "\"some string\"");
}

TEST(JsonStringBuilder, StringView) {
  StringBuilder sw;
  WriteToStream(std::string_view{"some string"}, sw);
  EXPECT_EQ(sw.GetString(), "\"some string\"");
}

TEST(JsonStringBuilder, Second) {
  StringBuilder sw;
  WriteToStream(std::chrono::seconds{42}, sw);
  EXPECT_EQ(sw.GetString(), "42");
}

TEST(JsonStringBuilder, EmptyOptional) {
  StringBuilder sw;
  WriteToStream(std::optional<int>{}, sw);
  EXPECT_EQ(sw.GetString(), "null");
}

TEST(JsonStringBuilder, FilledOptional) {
  StringBuilder sw;
  WriteToStream(std::optional<int>{42}, sw);
  EXPECT_EQ(sw.GetString(), "42");
}

template <typename T>
class JsonStringBuilderIntegralTypes : public ::testing::Test {};

using StringBuilderIntegralTypes =
    ::testing::Types<signed char, short, int, long, long long,

                     unsigned char, unsigned short, unsigned int, unsigned long,
                     unsigned long long,

                     char>;
TYPED_TEST_SUITE(JsonStringBuilderIntegralTypes, StringBuilderIntegralTypes);

TYPED_TEST(JsonStringBuilderIntegralTypes, Value) {
  TypeParam v = 42;

  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_EQ("42", sw.GetString());
}

TYPED_TEST(JsonStringBuilderIntegralTypes, Vector) {
  std::vector<TypeParam> v = {1, 2, 3};

  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_EQ("[1,2,3]", sw.GetString());
}

template <typename T>
class JsonStringBuilderFloatingTypesAndDeathTest : public ::testing::Test {};

using StringBuilderFloatingTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(JsonStringBuilderFloatingTypesAndDeathTest,
                 StringBuilderFloatingTypes);

TYPED_TEST(JsonStringBuilderFloatingTypesAndDeathTest, Value) {
  TypeParam v = 2.0;

  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_EQ("2.0", sw.GetString());
}

TYPED_TEST(JsonStringBuilderFloatingTypesAndDeathTest, Array) {
  std::array<TypeParam, 2> v = {4.0, 2.0};

  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_EQ("[4.0,2.0]", sw.GetString());
}

TYPED_TEST(JsonStringBuilderFloatingTypesAndDeathTest, Nan) {
  StringBuilder sw;

#ifdef NDEBUG
  EXPECT_THROW(WriteToStream(std::numeric_limits<TypeParam>::quiet_NaN(), sw),
               std::runtime_error);

  EXPECT_THROW(
      WriteToStream(std::numeric_limits<TypeParam>::signaling_NaN(), sw),
      std::runtime_error);

#else
  UEXPECT_DEATH(WriteToStream(std::numeric_limits<TypeParam>::quiet_NaN(), sw),
                "nan");
  UEXPECT_DEATH(
      WriteToStream(std::numeric_limits<TypeParam>::signaling_NaN(), sw),
      "nan");
#endif
}

TYPED_TEST(JsonStringBuilderFloatingTypesAndDeathTest, Inf) {
  StringBuilder sw;

#ifdef NDEBUG
  EXPECT_THROW(WriteToStream(std::numeric_limits<TypeParam>::infinity(), sw),
               std::runtime_error);
  EXPECT_THROW(WriteToStream(-std::numeric_limits<TypeParam>::infinity(), sw),
               std::runtime_error);

#else
  UEXPECT_DEATH(WriteToStream(std::numeric_limits<TypeParam>::infinity(), sw),
                "inf");
  UEXPECT_DEATH(WriteToStream(std::numeric_limits<TypeParam>::infinity(), sw),
                "inf");
#endif
}

/// [Sample formats::json::StringBuilder usage]
namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};

// The function must be declared in the namespace of your type
inline void WriteToStream(const MyKeyValue& data,
                          formats::json::StringBuilder& sw) {
  // A class that adds '{' in the constructor and '}' in the destructor to JSON
  formats::json::StringBuilder::ObjectGuard guard{sw};

  sw.Key("field1");
  // Use the WriteToStream functions for values, don't work with StringBuilder
  // directly
  WriteToStream(data.field1, sw);

  sw.Key("field2");
  WriteToStream(data.field2, sw);
}

TEST(JsonStringBuilder, ExampleUsage) {
  StringBuilder sb;
  MyKeyValue data = {"one", 1};
  WriteToStream(data, sb);
  ASSERT_EQ(sb.GetString(), "{\"field1\":\"one\",\"field2\":1}");
}

}  // namespace my_namespace
/// [Sample formats::json::StringBuilder usage]

USERVER_NAMESPACE_END
