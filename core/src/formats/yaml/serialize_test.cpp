#include <gtest/gtest.h>

#include <fstream>
#include <sstream>

#include <formats/yaml/exception.hpp>
#include <formats/yaml/serialize.hpp>

namespace {
constexpr const char* kDoc = "key1: 1\nkey2: val";
}

TEST(YamlSerialize, StringToString) {
  auto&& js_doc = formats::yaml::FromString(kDoc);
  EXPECT_EQ(formats::yaml::ToString(js_doc), kDoc);
}

TEST(YamlSerialize, StreamToString) {
  std::istringstream is(kDoc);
  auto&& js_doc = formats::yaml::FromStream(is);
  EXPECT_EQ(formats::yaml::ToString(js_doc), kDoc);
}

TEST(YamlSerialize, StringToStream) {
  auto&& js_doc = formats::yaml::FromString(kDoc);
  std::ostringstream os;
  formats::yaml::Serialize(js_doc, os);
  EXPECT_EQ(os.str(), kDoc);
}

TEST(YamlSerialize, StreamReadException) {
  std::fstream is("some-missing-doc");
  EXPECT_THROW(formats::yaml::FromStream(is),
               formats::yaml::BadStreamException);
}

TEST(YamlSerialize, StreamWriteException) {
  auto&& js_doc = formats::yaml::FromString(kDoc);
  std::ostringstream os;
  os.setstate(std::ios::failbit);
  EXPECT_THROW(formats::yaml::Serialize(js_doc, os),
               formats::yaml::BadStreamException);
}

TEST(YamlSerialize, ParsingException) {
  // Differs from JSON
  EXPECT_THROW(formats::yaml::FromString("&"), formats::yaml::ParseException);
}

TEST(YamlSerialize, EmptyDocException) {
  EXPECT_THROW(formats::yaml::FromString(""), formats::yaml::ParseException);
}
