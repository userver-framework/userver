#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>

namespace {
constexpr const char* kDoc = "{\"key1\":1,\"key2\":\"val\"}";
}

TEST(JsonSerialize, StringToString) {
  auto&& js_doc = formats::json::FromString(kDoc);
  EXPECT_EQ(formats::json::ToString(js_doc), kDoc);
}

TEST(JsonSerialize, StreamToString) {
  std::istringstream is(kDoc);
  auto&& js_doc = formats::json::FromStream(is);
  EXPECT_EQ(formats::json::ToString(js_doc), kDoc);
}

TEST(JsonSerialize, StringToStream) {
  auto&& js_doc = formats::json::FromString(kDoc);
  std::ostringstream os;
  formats::json::Serialize(js_doc, os);
  EXPECT_EQ(os.str(), kDoc);
}

TEST(JsonSerialize, StreamReadException) {
  std::fstream is("some-missing-doc");
  EXPECT_THROW(formats::json::FromStream(is),
               formats::json::BadStreamException);
}

TEST(JsonSerialize, StreamWriteException) {
  auto&& js_doc = formats::json::FromString(kDoc);
  std::ostringstream os;
  os.setstate(std::ios::failbit);
  EXPECT_THROW(formats::json::Serialize(js_doc, os),
               formats::json::BadStreamException);
}

TEST(JsonSerialize, ParsingException) {
  EXPECT_THROW(formats::json::FromString("\'"), formats::json::ParseException);
}

TEST(JsonSerialize, EmptyDocException) {
  EXPECT_THROW(formats::json::FromString(""), formats::json::ParseException);
}
