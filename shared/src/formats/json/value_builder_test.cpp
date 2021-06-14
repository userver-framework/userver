#include <gtest/gtest.h>

#include <formats/json/exception.hpp>
#include <formats/json/value_builder.hpp>

#include <formats/common/value_builder_test.hpp>

template <>
struct InstantiationDeathTest<formats::json::ValueBuilder>
    : public ::testing::Test {
  using ValueBuilder = formats::json::ValueBuilder;

  using Exception = formats::json::Exception;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, InstantiationDeathTest,
                               formats::json::ValueBuilder);
INSTANTIATE_TYPED_TEST_SUITE_P(FormatsJson, CommonValueBuilderTests,
                               formats::json::ValueBuilder);

TEST(JsonValueBuilder, ExampleUsage) {
  /// [Sample formats::json::ValueBuilder usage]
  // #include <formats/json.hpp>
  formats::json::ValueBuilder builder;
  builder["key1"] = 1;
  builder["key2"]["key3"] = "val";
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json["key1"].As<int>(), 1);
  ASSERT_EQ(json["key2"]["key3"].As<std::string>(), "val");
  /// [Sample formats::json::ValueBuilder usage]
}

/// [Sample Customization formats::json::ValueBuilder usage]
namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};

// The function must be declared in the namespace of your type
formats::json::Value Serialize(const MyKeyValue& data,
                               formats::serialize::To<formats::json::Value>) {
  formats::json::ValueBuilder builder;
  builder["field1"] = data.field1;
  builder["field2"] = data.field2;

  return builder.ExtractValue();
}

TEST(JsonValueBuilder, ExampleCustomization) {
  MyKeyValue object = {"val", 1};
  formats::json::ValueBuilder builder;
  builder["example"] = object;
  auto json = builder.ExtractValue();
  ASSERT_EQ(json["example"]["field1"].As<std::string>(), "val");
  ASSERT_EQ(json["example"]["field2"].As<int>(), 1);
}

}  // namespace my_namespace

/// [Sample Customization formats::json::ValueBuilder usage]
