#include <gtest/gtest.h>

#include <formats/json/serialize.hpp>
#include <formats/json/serialize_duration.hpp>
#include <formats/json/string_builder.hpp>
#include <formats/json/value_builder.hpp>

#include <cstring>

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

::formats::json::Value Serialize(
    const Response200EchoobjectwithmappingA& value,
    ::formats::serialize::To<::formats::json::Value>);

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
  ::formats::json::StringBuilder::ObjectGuard guard{sw};
}

void WriteToStream([[maybe_unused]] const Response200& value,
                   formats::json::StringBuilder& sw, bool hide_brackets,
                   [[maybe_unused]] const char* hide_field_name) {
  std::optional<::formats::json::StringBuilder::ObjectGuard> guard;
  if (!hide_brackets) guard.emplace(sw);

  if (!hide_field_name ||
      std::strcmp(hide_field_name, "object_no_mapping_type")) {
    sw.Key("object_no_mapping_type");

    WriteToStream(value.object_no_mapping_type, sw);
  }

  if (!hide_field_name ||
      std::strcmp(hide_field_name, "echoObjectWithMapping")) {
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
  sw.WriteUInt64(123u);

  EXPECT_EQ("123", sw.GetString());
}

TEST(JsonStringBuilder, Double) {
  StringBuilder sw;
  sw.WriteDouble(12.3);

  EXPECT_EQ("12.3", sw.GetString());
}

TEST(JsonStringBuilder, DoubleNan) {
  StringBuilder sw;
  EXPECT_THROW(WriteToStream(std::numeric_limits<double>::quiet_NaN(), sw),
               std::runtime_error);
  EXPECT_THROW(WriteToStream(std::numeric_limits<double>::signaling_NaN(), sw),
               std::runtime_error);
}

// TEST(JsonStringBuilder, DoubleInf) {
//  StringBuilder sw;
//  EXPECT_THROW(WriteToStream(std::numeric_limits<double>::infinity(), sw),
//               std::runtime_error);
//  EXPECT_THROW(WriteToStream(-std::numeric_limits<double>::infinity(), sw),
//               std::runtime_error);
//}

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

TEST(JsonStringBuilder, VectorInt) {
  std::vector<int> v = {1, 2, 3};
  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_EQ("[1,2,3]", sw.GetString());
}

TEST(JsonStringBuilder, VectorShort) {
  std::vector<short> v = {1, 2, 3};
  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_EQ("[1,2,3]", sw.GetString());
}

TEST(JsonStringBuilder, VectorUnsignedShort) {
  std::vector<unsigned short> v = {1, 2, 3};
  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_EQ("[1,2,3]", sw.GetString());
}

TEST(JsonStringBuilder, TestResponse) {
  testing::Response200 v{};
  StringBuilder sw;

  WriteToStream(v, sw);

  EXPECT_FALSE(sw.GetString().empty());
}

TEST(JsonStringBuilder, Float) {
  StringBuilder sw;
  WriteToStream(1.f, sw);

  EXPECT_EQ("1.0", sw.GetString());
}

TEST(JsonStringBuilder, FloatNan) {
  StringBuilder sw;
  EXPECT_THROW(WriteToStream(std::numeric_limits<float>::quiet_NaN(), sw),
               std::runtime_error);
  EXPECT_THROW(WriteToStream(std::numeric_limits<float>::signaling_NaN(), sw),
               std::runtime_error);
}

// TEST(JsonStringBuilder, FloatInf) {
//  StringBuilder sw;
//  EXPECT_THROW(WriteToStream(std::numeric_limits<float>::infinity(), sw),
//               std::runtime_error);
//  EXPECT_THROW(WriteToStream(-std::numeric_limits<float>::infinity(), sw),
//               std::runtime_error);
//}

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
