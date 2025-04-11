/// [json2yaml - unittest]
#include <json2yaml.hpp>

#include <gtest/gtest.h>

TEST(Json2Yaml, Basic) {
    namespace formats = USERVER_NAMESPACE::formats;

    auto json = formats::json::FromString(R"({"key": 42})");
    auto yaml = json.ConvertTo<formats::yaml::Value>();

    EXPECT_EQ(yaml["key"].As<int>(), 42);
}
/// [json2yaml - unittest]
