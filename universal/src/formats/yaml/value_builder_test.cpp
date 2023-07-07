#include <gtest/gtest.h>

#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/value_builder.hpp>

#include <formats/common/value_builder_test.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct InstantiationDeathTest<formats::yaml::ValueBuilder>
    : public ::testing::Test {
  using ValueBuilder = formats::yaml::ValueBuilder;

  using Exception = formats::yaml::Exception;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, InstantiationDeathTest,
                               formats::yaml::ValueBuilder);
INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, CommonValueBuilderTests,
                               formats::yaml::ValueBuilder);

TEST(YamlValueBuilder, ExampleUsage) {
  /// [Sample formats::yaml::ValueBuilder usage]
  // #include <userver/formats/yaml.hpp>
  formats::yaml::ValueBuilder builder;
  builder["key1"] = 1;
  formats::yaml::ValueBuilder sub_builder;
  sub_builder["key3"] = "val";
  builder["key2"] = sub_builder.ExtractValue();

  formats::yaml::Value yaml = builder.ExtractValue();

  ASSERT_EQ(yaml["key1"].As<int>(), 1);
  ASSERT_EQ(yaml["key2"]["key3"].As<std::string>(), "val");
  /// [Sample formats::yaml::ValueBuilder usage]
}

/// [Sample Customization formats::yaml::ValueBuilder usage]
namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};

// The function must be declared in the namespace of your type
formats::yaml::Value Serialize(const MyKeyValue& data,
                               formats::serialize::To<formats::yaml::Value>) {
  formats::yaml::ValueBuilder builder;
  builder["field1"] = data.field1;
  builder["field2"] = data.field2;

  return builder.ExtractValue();
}

TEST(YamlValueBuilder, ExampleCustomization) {
  MyKeyValue object = {"val", 1};
  formats::yaml::ValueBuilder builder;
  builder["example"] = object;
  auto yaml = builder.ExtractValue();
  ASSERT_EQ(yaml["example"]["field1"].As<std::string>(), "val");
  ASSERT_EQ(yaml["example"]["field2"].As<int>(), 1);
}

}  // namespace my_namespace

/// [Sample Customization formats::yaml::ValueBuilder usage]

TEST(YamlValueBuilder, MultiSegmentPath) {
  formats::yaml::ValueBuilder builder(formats::common::Type::kObject);
  builder["foo"]["bar"] = "baz";
  ASSERT_TRUE(builder["foo"].HasMember("bar"));
}

USERVER_NAMESPACE_END
