#include <gtest/gtest.h>
/// [Sample Customization formats::*::ValueBuilder usage]
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
Value Serialize(const MyKeyValue& data, formats::serialize::To<Value>) {
    typename Value::Builder builder;
    builder["field1"] = data.field1;
    builder["field2"] = data.field2;

    return builder.ExtractValue();
}

TEST(CommonFormats, Serialize) {
    const MyKeyValue obj = {"val", 1};

    // json
    formats::json::ValueBuilder builder_json;
    builder_json["example"] = obj;
    auto json = builder_json.ExtractValue();
    EXPECT_EQ(json["example"]["field1"].As<std::string>(), "val");
    EXPECT_EQ(json["example"]["field2"].As<int>(), 1);

    // yaml
    formats::yaml::ValueBuilder builder_yaml;
    builder_yaml["example"] = obj;
    auto yaml = builder_yaml.ExtractValue();
    EXPECT_EQ(yaml["example"]["field1"].As<std::string>(), "val");
    EXPECT_EQ(yaml["example"]["field2"].As<int>(), 1);
}

}  // namespace my_namespace
   /// [Sample Customization formats::*::ValueBuilder usage]

USERVER_NAMESPACE_END
