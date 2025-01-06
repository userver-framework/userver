#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/serialize_container.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>
#include <userver/utest/literals.hpp>

#include <gmock/gmock-matchers.h>

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
MyKeyValue Parse(const formats::yaml::Value& yaml, formats::parse::To<MyKeyValue>) {
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

TEST(FormatsYaml, NonObjectHasMember) {
    formats::yaml::ValueBuilder builder;
    builder["key1"] = 1;
    ASSERT_THROW(builder["key1"].HasMember("key2"), formats::yaml::TypeMismatchException);
}

TEST(FormatsYaml, UserDefinedLiterals) {
    using ValueBuilder = formats::yaml::ValueBuilder;
    ValueBuilder builder{formats::common::Type::kObject};
    builder["test"] = 3;
    EXPECT_EQ(
        builder.ExtractValue(),
        R"yaml(
    test: 3
    )yaml"_yaml
    );
}

TEST(FormatsYaml, NodesTags) {
    formats::yaml::Value yaml = formats::yaml::FromString(R"(
# Indenting matters
%TAG !example! tag:example,2024:

    field_int: 10
    field_string: bar
    field_bool: false
    field_array:
    - 1
    field_obj:
      foo: bar
    field_array_json: [1, 2]
    field_obj_json:  {"foo": "bar"}

    field_null: null

    field_string_quote: 'bar'
    field_block_string: |-
      123

    field_custom_tag: !my_tag
      foo: bar
    field_custom_tag_null: !my_tag null
    field_custom_tag_string: !my_tag
    field_internal_tag: !!str '123'
    field_tag_prefix: !example!foo 42
  )");

    for (auto scalar_field :
         {"field_int",
          "field_string",
          "field_bool",
          "field_array",
          "field_obj",
          "field_array_json",
          "field_obj_json"}) {
        EXPECT_EQ(yaml[scalar_field].GetTag(), "?") << "Field: " << scalar_field;
    }

    EXPECT_EQ(yaml["field_null"].GetTag(), "");
    EXPECT_EQ(yaml["field_string_quote"].GetTag(), "!");
    EXPECT_EQ(yaml["field_block_string"].GetTag(), "!");

    EXPECT_EQ(yaml["field_custom_tag"].GetTag(), "!my_tag");
    // NOTE: Old versions of yaml-cpp "erase" explicit tag for null values
    // 0.5.3 -- Empty string ""
    // 0.8.0 -- Specified tag "!my_tag"
    EXPECT_THAT(yaml["field_custom_tag_null"].GetTag(), testing::AnyOf(testing::Eq(""), testing::Eq("!my_tag")));
    EXPECT_EQ(yaml["field_custom_tag_string"].GetTag(), "!my_tag");
    EXPECT_EQ(yaml["field_internal_tag"].GetTag(), "tag:yaml.org,2002:str");
    EXPECT_EQ(yaml["field_tag_prefix"].GetTag(), "tag:example,2024:foo");
}

USERVER_NAMESPACE_END
