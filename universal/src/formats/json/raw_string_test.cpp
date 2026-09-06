#include <userver/formats/json/raw_string.hpp>

#include <gtest/gtest.h>

#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

TEST(RawString, ConstructFromJson) {
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

    const formats::json::RawString raw_string(json);

    EXPECT_EQ(raw_string.GetView(), R"({"a":"foo","b":{"c":"d","e":[1,2]}})");
}

TEST(RawString, ConstructFromString) {
    const std::string json = R"({"a":"foo",
"b":{"c":"d","e":
[1,2]}})";

    const formats::json::RawString raw_string(json);

    EXPECT_EQ(raw_string.GetView(), json);
}

TEST(RawString, ConstructEmpty) {
    const formats::json::RawString raw_string;

    EXPECT_EQ(raw_string.GetView(), "{}");
}

TEST(RawString, Equality) {
    EXPECT_EQ(formats::json::RawString(R"({"a":1})"), formats::json::RawString(R"({"a":1})"));
    EXPECT_FALSE(formats::json::RawString(R"({"a":1})") == formats::json::RawString(R"({"a":2})"));
}

TEST(RawString, SerializeToValue) {
    const formats::json::RawString raw_string(R"({"a":1})");
    const auto value = Serialize(raw_string, formats::serialize::To<formats::json::Value>{});
    EXPECT_EQ(value, formats::json::FromString(R"({"a":1})"));
}

TEST(RawString, ParseToRawString) {
    const auto
        raw_string = Parse(formats::json::FromString(R"({"a":1})"), formats::parse::To<formats::json::RawString>{});
    EXPECT_EQ(raw_string.GetView(), R"({"a":1})");
}

USERVER_NAMESPACE_END
