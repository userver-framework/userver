#include <gtest/gtest.h>

#include <formats/json/serialize.hpp>
#include <formats/json/string_builder.hpp>
#include <formats/json/value_builder.hpp>

using formats::json::FromString;
using formats::json::ValueBuilder;
using formats::json::impl::StringBuilder;

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
