#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>

#include <schemas/raw_json_string.hpp>

USERVER_NAMESPACE_BEGIN

TEST(RawJsonString, RawJsonField) {
    const auto json = formats::json::MakeObject("inner_object", formats::json::FromString(R"({"foo":1,"bar":[2,3]})"));
    const auto obj = json.As<ns::ObjectWithRawJsonField>();

    static_assert(std::is_same_v<decltype(obj.inner_object), formats::json::RawString>);
    EXPECT_EQ(obj.inner_object.GetView(), R"({"foo":1,"bar":[2,3]})");

    const auto roundtrip = formats::json::ValueBuilder(obj).ExtractValue();
    EXPECT_EQ(roundtrip["inner_object"], formats::json::FromString(R"({"foo":1,"bar":[2,3]})"));
}

TEST(RawJsonString, RawJsonStringObject) {
    const auto json = formats::json::FromString(R"({"foo":1,"bar":[2,3]})");
    const auto obj = json.As<ns::RawJsonStringObject>();

    static_assert(std::is_same_v<decltype(obj), const formats::json::RawString>);
    EXPECT_EQ(obj.GetView(), R"({"foo":1,"bar":[2,3]})");

    const auto roundtrip = formats::json::ValueBuilder(obj).ExtractValue();
    EXPECT_EQ(roundtrip, json);
}

USERVER_NAMESPACE_END
