#include <gtest/gtest.h>
/// [Sample formats::*::Value::As<T>() usage]
#include <userver/formats/json.hpp>
#include <userver/formats/yaml.hpp>

USERVER_NAMESPACE_BEGIN

namespace my_namespace {

struct MyKeyValue {
  std::string field1;
  int field2;
};

// The function must be declared in the namespace of your type
template <class Value>
MyKeyValue Parse(const Value& data, formats::parse::To<MyKeyValue>) {
  return MyKeyValue{
      data["field1"].template As<std::string>(),
      data["field2"].template As<int>(-1),  // return `1` if "field2" is missing
  };
}

TEST(CommonFormats, Parse) {
  // json
  formats::json::Value json = formats::json::FromString(R"({
    "my_value": {
        "field1": "one",
        "field2": 1
    }
  })");
  auto data_json = json["my_value"].As<MyKeyValue>();
  EXPECT_EQ(data_json.field1, "one");
  EXPECT_EQ(data_json.field2, 1);

  // yaml
  formats::yaml::Value yaml = formats::yaml::FromString(R"(
    my_value:
        field1: "one"
        field2: 1
  )");
  auto data_yaml = yaml["my_value"].As<MyKeyValue>();
  EXPECT_EQ(data_yaml.field1, "one");
  EXPECT_EQ(data_yaml.field2, 1);
}

}  // namespace my_namespace
/// [Sample formats::*::Value::As<T>() usage]

USERVER_NAMESPACE_END
