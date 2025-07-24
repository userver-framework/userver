#include <userver/logging/json_string.hpp>

#include <gtest/gtest.h>

#include <userver/formats/json/value.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

TEST(JsonString, ConstructFromJson) {
    using formats::literals::operator""_json;

    auto json = R"({
      "a": "foo",
      "b": {
        "c": "d",
        "e": [
          1,
          2
        ]
      }
    })"_json;

    const logging::JsonString json_string(json);

    EXPECT_EQ(json_string.GetView(), R"({"a":"foo","b":{"c":"d","e":[1,2]}})");
}

TEST(JsonString, ConstructFromString) {
    const std::string json = R"({"a":"foo",
"b":{"c":"d","e":
[1,2]}})";

    const logging::JsonString json_string(json);

    EXPECT_EQ(json_string.GetView(), R"({"a":"foo","b":{"c":"d","e":[1,2]}})");
}

TEST(JsonString, ConstructNull) {
    const logging::JsonString json_string;

    EXPECT_EQ(json_string.GetView(), R"(null)");
}

USERVER_NAMESPACE_END
