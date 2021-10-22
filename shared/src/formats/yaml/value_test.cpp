#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/serialize_container.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

#include <formats/common/value_test.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct Parsing<formats::yaml::Value> : public ::testing::Test {
  constexpr static auto FromString = formats::yaml::FromString;
  using ParseException = formats::yaml::Value::ParseException;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, Parsing, formats::yaml::Value);

TEST(FormatsYaml, NullAsDefaulted) {
  using formats::yaml::FromString;
  auto yaml = FromString(R"({"nulled": ~})");

  EXPECT_EQ(yaml["nulled"].As<int>({}), 0);
  EXPECT_EQ(yaml["nulled"].As<std::vector<int>>({}), std::vector<int>{});

  EXPECT_EQ(yaml["nulled"].As<int>(42), 42);

  std::vector<int> value{4, 2};
  EXPECT_EQ(yaml["nulled"].As<std::vector<int>>(value), value);
}

TEST(FormatsYaml, ExampleUsage) {
  /// [Sample formats::yaml::Value usage]
  // #include <userver/formats/yaml.hpp>

  formats::yaml::Value yaml = formats::yaml::FromString(R"(
  key1: 1
  key2:
      key3: "val"
  )");

  const auto key1 = yaml["key1"].As<int>();
  ASSERT_EQ(key1, 1);

  const auto key3 = yaml["key2"]["key3"].As<std::string>();
  ASSERT_EQ(key3, "val");
  /// [Sample formats::yaml::Value usage]
}

/// [Sample formats::yaml::Value::As<T>() usage]
namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};
//  The function must be declared in the namespace of your type
MyKeyValue Parse(const formats::yaml::Value& yaml,
                 formats::parse::To<MyKeyValue>) {
  return MyKeyValue{
      yaml["field1"].As<std::string>(""),
      yaml["field2"].As<int>(1),  // return `1` if "field2" is missing
  };
}

TEST(FormatsYaml, ExampleUsageMyStruct) {
  formats::yaml::Value yaml = formats::yaml::FromString(R"(
    my_value:
      field1: "one"
      field2: 1
  )");

  auto data = yaml["my_value"].As<MyKeyValue>();
  EXPECT_EQ(data.field1, "one");
  EXPECT_EQ(data.field2, 1);
}
}  // namespace my_namespace
/// [Sample formats::yaml::Value::As<T>() usage]

USERVER_NAMESPACE_END
