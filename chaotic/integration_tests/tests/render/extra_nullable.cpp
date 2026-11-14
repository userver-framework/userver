#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>

#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include <schemas/extra_nullable.hpp>
#include <schemas/extra_nullable_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using NullableNumbers = std::unordered_map<std::string, std::optional<double>>;

const auto kJson = formats::json::FromString(R"({"a":1.5,"b":null})");

}  // namespace

TEST(ExtraNullable, MemberType) {
    static_assert(std::is_same_v<decltype(ns::NullableNumbers::extra), NullableNumbers>);
}

TEST(ExtraNullable, ParsesNulls) {
    for (const auto& object : {kJson.As<ns::NullableNumbers>(), CallSaxParser<ns::NullableNumbers>(ToString(kJson))}) {
        EXPECT_EQ(object.extra, (NullableNumbers{{"a", 1.5}, {"b", std::nullopt}}));
    }
}

TEST(ExtraNullable, WritesNulls) {
    const auto object = kJson.As<ns::NullableNumbers>();
    for (const auto& json : {TestDomSerializer(object), TestToJsonString(object), TestWriteToStream(object)}) {
        EXPECT_EQ(json, kJson);
    }
}

TEST(ExtraNullable, EmptyMapIsAnObject) {
    const ns::NullableNumbers object;
    for (const auto& json : {TestDomSerializer(object), TestToJsonString(object), TestWriteToStream(object)}) {
        EXPECT_EQ(json, formats::json::MakeObject());
    }
}

TEST(ExtraNullable, NextToAField) {
    const auto json = formats::json::FromString(R"({"name":"x","a":null})");
    for (const auto& object :
         {json.As<ns::NullableNumbersWithField>(), CallSaxParser<ns::NullableNumbersWithField>(ToString(json))})
    {
        EXPECT_EQ(object.name, "x");
        EXPECT_EQ(object.extra, (NullableNumbers{{"a", std::nullopt}}));
        EXPECT_EQ(TestDomSerializer(object), json);
        EXPECT_EQ(TestWriteToStream(object), json);
    }
}

TEST(ExtraNullable, WrongTypeIsRefused) {
    const auto json = formats::json::FromString(R"({"a":"text"})");
    UEXPECT_THROW(json.As<ns::NullableNumbers>(), std::exception);
    UEXPECT_THROW(CallSaxParser<ns::NullableNumbers>(ToString(json)), std::exception);
}

USERVER_NAMESPACE_END
