#include <userver/logging/json_string.hpp>

#include <gtest/gtest.h>

#include <userver/utest/assert_macros.hpp>


USERVER_NAMESPACE_BEGIN

TEST(JsonString, ConstructFromJson) {
  using namespace formats::literals;

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

  logging::JsonString json_string(json);

  EXPECT_EQ(json_string.Value(), R"({"a":"foo","b":{"c":"d","e":[1,2]}})");
}

TEST(JsonString, ConstructFromString) {
  std::string json = R"({"a":"foo",
"b":{"c":"d","e":
[1,2]}})";

  logging::JsonString json_string(json);

  EXPECT_EQ(json_string.Value(), R"({"a":"foo","b":{"c":"d","e":[1,2]}})");
}

TEST(JsonString, ConstructNull) {
  logging::JsonString json_string;

  EXPECT_EQ(json_string.Value(), R"(null)");
}


USERVER_NAMESPACE_END
