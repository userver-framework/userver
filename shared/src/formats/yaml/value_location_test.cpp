#include <gtest/gtest.h>

#include <userver/formats/yaml.hpp>

USERVER_NAMESPACE_BEGIN

namespace my_namespace {

namespace {

std::pair<int, int> LocationPair(const formats::yaml::Value& data) {
  return std::make_pair(data.GetLine(), data.GetColumn());
}

}  // namespace

TEST(FormatsYaml, NativeLocation) {
  formats::yaml::Value yaml = formats::yaml::FromString(R"(
    my_value:
        field1: "one"
        field2: 1
  )");

  EXPECT_EQ(LocationPair(yaml), std::pair(1, 4));
  EXPECT_EQ(LocationPair(yaml["my_value"]), std::pair(2, 8));
  EXPECT_EQ(LocationPair(yaml["my_value"]["field1"]), std::pair(2, 16));
  EXPECT_EQ(LocationPair(yaml["my_value"]["field2"]), std::pair(3, 16));

  EXPECT_EQ(LocationPair(yaml["missing_value"]), std::pair(-1, -1));
}

TEST(FormatsYaml, ValueBuilderLocation) {
  formats::yaml::ValueBuilder builderExample;
  builderExample["field1"] = "one";
  builderExample["field2"] = 1;

  formats::yaml::ValueBuilder builderYaml;
  builderYaml["my_value"] = builderExample.ExtractValue();

  auto yaml = builderYaml.ExtractValue();

  EXPECT_EQ(LocationPair(yaml), std::pair(0, 0));
  EXPECT_EQ(LocationPair(yaml["my_value"]), std::pair(0, 0));
  EXPECT_EQ(LocationPair(yaml["my_value"]["field1"]), std::pair(0, 0));
  EXPECT_EQ(LocationPair(yaml["my_value"]["field2"]), std::pair(0, 0));

  EXPECT_EQ(LocationPair(yaml["missing_value"]), std::pair(-1, -1));
}

}  // namespace my_namespace

USERVER_NAMESPACE_END
