#include <gtest/gtest.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/value_builder.hpp>

// for testing std::optional/null
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>

#include <formats/common/value_builder_test.hpp>

USERVER_NAMESPACE_BEGIN

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
  // #include <userver/formats/json.hpp>
  formats::json::ValueBuilder builder;
  builder["key1"] = 1;
  builder["key2"]["key3"] = "val";
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json["key1"].As<int>(), 1);
  ASSERT_EQ(json["key2"]["key3"].As<std::string>(), "val");
  /// [Sample formats::json::ValueBuilder usage]
}

TEST(JsonValueBuilder, ValueString) {
  formats::json::ValueBuilder builder;
  builder = "abc";
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json.As<std::string>(), "abc");
  ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueEmptyString) {
  formats::json::ValueBuilder builder;
  builder = "";
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json.As<std::string>(), "");
  ASSERT_THROW(json.As<std::vector<std::string>>(),
               formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueNumber) {
  formats::json::ValueBuilder builder;
  builder = 321;
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json.As<int>(), 321);
  ASSERT_THROW(json.As<bool>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueTrue) {
  formats::json::ValueBuilder builder;
  builder = true;
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json.As<bool>(), true);
  ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueFalse) {
  formats::json::ValueBuilder builder = false;
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json.As<bool>(), false);
  ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);
}

TEST(JsonValueBuilder, ValueNull) {
  formats::json::ValueBuilder builder;
  builder = std::optional<int>{};
  formats::json::Value json = builder.ExtractValue();

  ASSERT_EQ(json.As<std::optional<int>>(), std::nullopt);
  ASSERT_EQ(json.As<std::optional<std::vector<std::string>>>(), std::nullopt);
  ASSERT_THROW(json.As<int>(), formats::json::TypeMismatchException);

  formats::json::ValueBuilder builder_def;
  formats::json::Value json_def = builder_def.ExtractValue();

  ASSERT_EQ(json_def.As<std::optional<std::string>>(), std::nullopt);
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

USERVER_NAMESPACE_END
