#include <schemas/enum_fallback.hpp>
#include <schemas/enum_fallback_sax_parsers.hpp>

#include <exception>
#include <string>
#include <type_traits>

#include <userver/chaotic/convert.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

static_assert(std::is_enum_v<ns::FallbackEnum>);

TEST(EnumFallback, OldReaderAcceptsNewProducer) {
    /// [enum fallback parse]
    const auto value = formats::json::FromString(R"("future")").As<ns::FallbackEnum>();
    EXPECT_EQ(value, ns::FallbackEnum::kUnknown);
    EXPECT_EQ(ns::ToString(value), "unknown");
    /// [enum fallback parse]
    EXPECT_EQ(formats::json::FromString(R"("order")").As<ns::FallbackEnum>(), ns::FallbackEnum::kOrder);
    EXPECT_EQ(chaotic::ConvertTo<ns::FallbackEnum>(std::string{"future"}), ns::FallbackEnum::kUnknown);
    EXPECT_EQ(ns::Parse("future", formats::parse::To<ns::FallbackEnum>{}), ns::FallbackEnum::kUnknown);
    EXPECT_FALSE(ns::TryConvert("future", chaotic::convert::To<ns::FallbackEnum>{}));
    EXPECT_EQ(TestDomSerializer(value).As<std::string>(), "unknown");
    EXPECT_EQ(TestWriteToStream(value).As<std::string>(), "unknown");
}

TEST(EnumFallback, BothAliasesAndStrictEnums) {
    const auto unknown = formats::json::FromString(R"("future")");
    EXPECT_EQ(unknown.As<ns::TaxiFallbackEnum>(), ns::TaxiFallbackEnum::kOther);
    EXPECT_EQ(CallSaxParser<ns::TaxiFallbackEnum>(R"("future")"), ns::TaxiFallbackEnum::kOther);
    EXPECT_THROW(unknown.As<ns::ClosedEnum>(), std::exception);
    EXPECT_THROW(unknown.As<ns::DisabledFallbackEnum>(), std::exception);
    EXPECT_THROW(CallSaxParser<ns::ClosedEnum>(R"("future")"), std::exception);
    EXPECT_THROW(CallSaxParser<ns::DisabledFallbackEnum>(R"("future")"), std::exception);
    EXPECT_TRUE(formats::parse::MayParseAs<ns::FallbackEnum>(unknown));
    EXPECT_TRUE(formats::parse::MayParseAs<ns::TaxiFallbackEnum>(unknown));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::ClosedEnum>(unknown));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::DisabledFallbackEnum>(unknown));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::FallbackEnum>(formats::json::FromString("42")));
}

TEST(EnumFallback, ObjectSaxDefaultsAndNullable) {
    const auto json = formats::json::FromString(R"({"value":"future","history":["order","future"],"nullable":"future"})"
    );
    const auto value = json.As<ns::FallbackEvent>();
    EXPECT_EQ(value.value, ns::FallbackEnum::kUnknown);
    ASSERT_TRUE(value.history);
    EXPECT_EQ(value.history->at(0), ns::FallbackEnum::kOrder);
    EXPECT_EQ(value.history->at(1), ns::FallbackEnum::kUnknown);
    EXPECT_EQ(value.defaulted, ns::FallbackEvent::Defaulted::kShipment);
    ASSERT_TRUE(value.nullable);
    EXPECT_EQ(*value.nullable, ns::FallbackEvent::Nullable::kUnknown);
    EXPECT_EQ(ns::FromJsonString(formats::json::ToString(json), formats::parse::To<ns::FallbackEvent>{}), value);
    EXPECT_TRUE(formats::parse::MayParseAs<ns::FallbackEvent>(json));
    EXPECT_EQ(TestWriteToStream(value), TestDomSerializer(value));
    EXPECT_EQ(TestToJsonString(value), TestDomSerializer(value));
    const auto null_json = formats::json::FromString(R"({"value":"order","nullable":null,"defaulted":"future"})");
    const auto null_value = null_json.As<ns::FallbackEvent>();
    EXPECT_FALSE(null_value.nullable);
    EXPECT_EQ(null_value.defaulted, ns::FallbackEvent::Defaulted::kUnknown);
    EXPECT_EQ(CallSaxParser<ns::FallbackEvent>(formats::json::ToString(null_json)), null_value);
}

TEST(EnumFallback, MissingAndWrongTypeStillFail) {
    for (const auto input : {R"({})", R"({"value":null})", R"({"value":42})", R"({"value":[]})"}) {
        EXPECT_THROW(formats::json::FromString(input).As<ns::FallbackEvent>(), std::exception);
        EXPECT_THROW(CallSaxParser<ns::FallbackEvent>(input), std::exception);
    }
}

TEST(EnumFallback, YamlAndYamlConfig) {
    const auto yaml = formats::yaml::FromString("value: future\nnullable: null");
    EXPECT_EQ(yaml.As<ns::FallbackEvent>().value, ns::FallbackEnum::kUnknown);
    const yaml_config::YamlConfig config{yaml, {}};
    EXPECT_EQ(config.As<ns::FallbackEvent>().value, ns::FallbackEnum::kUnknown);
}

TEST(EnumFallback, NullableClosedEnum) {
    const auto json = formats::json::FromString(R"({"value":null})");
    const auto value = json.As<ns::StrictNullableEnumEvent>();
    EXPECT_FALSE(value.value);
    EXPECT_EQ(CallSaxParser<ns::StrictNullableEnumEvent>(formats::json::ToString(json)), value);
    const auto unknown = R"({"value":"future"})";
    EXPECT_THROW(formats::json::FromString(unknown).As<ns::StrictNullableEnumEvent>(), std::exception);
    EXPECT_THROW(CallSaxParser<ns::StrictNullableEnumEvent>(unknown), std::exception);
}

USERVER_NAMESPACE_END
