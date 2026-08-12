#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/variant.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>

#include <schemas/object_default_additional_properties.hpp>
#include <schemas/object_single_field.hpp>
#include <schemas/object_single_field_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, Integer) {
    auto json = formats::json::MakeObject("int3", 1, "integer", 3);
    auto obj = json.As<ns::SimpleObject>();
    EXPECT_EQ(obj.integer, 3);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
    EXPECT_EQ(TestWriteToStream(obj), TestDomSerializer(obj));
}

TEST(Simple, IntegerSax) {
    auto json = formats::json::MakeObject("int3", 1, "integer", 3);
    auto obj = CallSaxParser<ns::SimpleObject>(ToString(json));
    EXPECT_EQ(obj.integer, 3);
    EXPECT_EQ(TestWriteToStream(obj), TestDomSerializer(obj));
}

TEST(Simple, DefaultFieldValue) {
    const ns::SimpleObject obj;
    EXPECT_EQ(obj.int_, 1);
}

TEST(Simple, IntegerMinimum) {
    auto json = formats::json::MakeObject("int3", 1, "int", -10);
    UEXPECT_THROW_MSG(
        json.As<ns::SimpleObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'int': Invalid value, minimum=-1, given=-10"
    );
}

TEST(Simple, IntegerMaximum) {
    auto json = formats::json::MakeObject("int3", 1, "int", 11);
    UEXPECT_THROW_MSG(
        json.As<ns::SimpleObject>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'int': Invalid value, maximum=10, given=11"
    );
}

TEST(Simple, ObjectDefault) {
    auto json = formats::json::MakeObject("int3", 1);
    auto obj = json.As<ns::SimpleObject>();
    EXPECT_EQ(obj.int_, 1);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, ObjectRequired) {
    auto json = formats::json::MakeObject();
    UEXPECT_THROW_MSG(
        json.As<ns::SimpleObject>(),
        formats::json::MemberMissingException,
        "Error at path 'int3': Field is missing"
    );
}

TEST(Simple, IntegerFormat) {
    static_assert(std::is_same_v<decltype(ns::SimpleObject::integer), std::optional<int>>);
    static_assert(std::is_same_v<decltype(ns::SimpleObject::int_), std::int32_t>);
    static_assert(std::is_same_v<decltype(ns::SimpleObject::int3), std::int64_t>);
}

TEST(Simple, ObjectWithRefType) {
    auto json = formats::json::MakeObject("integer", 0);
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectWithRef>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'integer': Invalid value, minimum=1, given=0"
    );
}

TEST(Simple, ObjectTypes) {
    auto json = formats::json::MakeObject(
        "integer",
        1,
        "boolean",
        true,
        "number",
        1.1,
        "string",
        "foo",
        "string-enum",
        "1",
        "object",
        formats::json::MakeObject(),
        "array",
        formats::json::MakeArray(1)
    );
    auto obj = json.As<ns::ObjectTypes>();
    EXPECT_EQ(
        obj,
        (ns::ObjectTypes{true, 1, 1.1, "foo", ns::ObjectTypes::Object{}, {1}, {}, ns::ObjectTypes::String_Enum::kX1})
    );
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, ObjectWithAdditionalPropertiesInt) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3);
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesInt>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, (std::unordered_map<std::string, int>{{"two", 2}, {"three", 3}}));
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, ParseObjectWithAdditionalPropertiesIntFromYaml) {
    auto yaml = formats::yaml::FromString(R"(
one: 1
two: 2
three: 3
    )");
    auto obj = yaml.As<ns::ObjectWithAdditionalPropertiesInt>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, (std::unordered_map<std::string, int>{{"two", 2}, {"three", 3}}));
}

TEST(Simple, ObjectWithAdditionalPropertiesIntMinimum) {
    auto json = formats::json::MakeObject("one", 1, "two", 1);
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectWithAdditionalPropertiesInt>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'two': Invalid value, minimum=2, given=1"
    );
}

TEST(Simple, ObjectWithAdditionalPropertiesIntSax) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3);
    auto obj = CallSaxParser<ns::ObjectWithAdditionalPropertiesInt>(ToString(json));

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, (std::unordered_map<std::string, int>{{"two", 2}, {"three", 3}}));
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, ObjectWithAdditionalPropertiesDefault) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = json.As<ns::ObjectDefaultAdditionalProperties>();

    EXPECT_EQ(obj.one, 1);

    ns::ObjectDefaultAdditionalProperties test;
    test.one = 1;
    EXPECT_EQ(obj, test);
}

TEST(Simple, ObjectWithAdditionalPropertiesTrue) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesTrue>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, formats::json::MakeObject("two", 2, "three", 3, "object", formats::json::MakeObject()));
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, ParseObjectWithAdditionalPropertiesTrueFromYaml) {
    auto yaml = formats::yaml::FromString(R"(
one: 1
two: 2
three: 3
object: {}
        )");
    auto obj = yaml.As<ns::ObjectWithAdditionalPropertiesTrue>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, formats::json::MakeObject("two", 2, "three", 3, "object", formats::json::MakeObject()));
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, ObjectWithAdditionalPropertiesTrueSax) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = CallSaxParser<ns::ObjectWithAdditionalPropertiesTrue>(ToString(json));

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, formats::json::MakeObject("two", 2, "three", 3, "object", formats::json::MakeObject()));
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, ObjectExtraMemberFalse) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesTrueExtraMemberFalse>();

    EXPECT_EQ(obj.one, 1);

    EXPECT_EQ(TestDomSerializer(obj), formats::json::MakeObject("one", 1));
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, ObjectExtraMemberFalseSax) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = CallSaxParser<ns::ObjectWithAdditionalPropertiesTrueExtraMemberFalse>(ToString(json));

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, ObjectWithAdditionalPropertiesFalseStrict) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectWithAdditionalPropertiesFalseStrict>(),
        chaotic::Error<formats::json::Value>,
        "Unknown property 'bar'"
    );
}

TEST(Simple, ObjectWithAdditionalPropertiesFalseStrictSax) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    UEXPECT_THROW_MSG(
        CallSaxParser<ns::ObjectWithAdditionalPropertiesFalseStrict>(ToString(json)),
        formats::json::parser::ParseError,
        "Parse error at pos 14, path 'bar': unknown field 'bar' for type "
        "'ns::ObjectWithAdditionalPropertiesFalseStrict', the latest token was ,\"bar\""
    );
}

TEST(Simple, IntegerEnum) {
    auto json = formats::json::MakeObject("one", 1);
    auto obj = json["one"].As<ns::IntegerEnum>();

    EXPECT_EQ(obj, ns::IntegerEnum::k1);
    EXPECT_EQ(TestWriteToStream(obj), json["one"]);

    EXPECT_EQ(TestDomSerializer(obj), json["one"]);

    auto json2 = formats::json::MakeObject("one", 5);
    UEXPECT_THROW_MSG(
        json2["one"].As<ns::IntegerEnum>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'one': Invalid enum value (5) for type ::ns::IntegerEnum"
    );

    EXPECT_EQ(std::size(ns::kIntegerEnumValues), 3);

    const ns::IntegerEnum values[] = {ns::IntegerEnum::k1, ns::IntegerEnum::k2, ns::IntegerEnum::k3};
    std::size_t index = 0;
    for (const auto& value : ns::kIntegerEnumValues) {
        EXPECT_EQ(value, values[index]);
        EXPECT_EQ(chaotic::ConvertTo<ns::IntegerEnum>(value), value);
        EXPECT_EQ(chaotic::ConvertTo<ns::IntegerEnum>(static_cast<std::int64_t>(value)), value);
        EXPECT_EQ(Convert(static_cast<std::int64_t>(value), chaotic::convert::To<ns::IntegerEnum>{}), value);
        EXPECT_EQ(TryConvert(static_cast<std::int64_t>(value), chaotic::convert::To<ns::IntegerEnum>{}), value);

        ++index;
    }
}

TEST(Simple, IntegerEnumSax) {
    auto obj = CallSaxParser<ns::IntegerEnum>("1");
    EXPECT_EQ(obj, ns::IntegerEnum::k1);

    EXPECT_EQ(TestDomSerializer(obj).As<int>(), 1);
    EXPECT_EQ(TestWriteToStream(obj), TestDomSerializer(obj));

    auto json2 = formats::json::MakeObject("one", 5);
    UEXPECT_THROW_MSG(
        CallSaxParser<ns::IntegerEnum>("5"),
        formats::json::parser::ParseError,
        "Parse error at pos 1, path '': Invalid enum value (5) for type ::ns::IntegerEnum, the latest token was 5"
    );
}

TEST(Simple, StringEnum) {
    auto json = formats::json::MakeObject("one", "foo");
    auto obj = json["one"].As<ns::StringEnum>();
    EXPECT_EQ(obj, ns::StringEnum::kFoo);
    EXPECT_EQ(TestWriteToStream(obj), json["one"]);

    EXPECT_EQ(TestDomSerializer(obj), json["one"]);

    auto json2 = formats::json::MakeObject("one", "zoo");
    UEXPECT_THROW_MSG(
        json2["one"].As<ns::StringEnum>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'one': Invalid enum value (zoo) for type ::ns::StringEnum"
    );

    EXPECT_EQ("foo", ToString(ns::StringEnum::kFoo));
    EXPECT_EQ("bar", ToString(ns::StringEnum::kBar));
    EXPECT_EQ("some!thing", ToString(ns::StringEnum::kSomeThing));

    EXPECT_EQ(TryConvert("foo", chaotic::convert::To<ns::StringEnum>{}), ns::StringEnum::kFoo);
    EXPECT_EQ(TryConvert("bar", chaotic::convert::To<ns::StringEnum>{}), ns::StringEnum::kBar);
    EXPECT_EQ(TryConvert("some!thing", chaotic::convert::To<ns::StringEnum>{}), ns::StringEnum::kSomeThing);
    EXPECT_FALSE(TryConvert("invalid", chaotic::convert::To<ns::StringEnum>{}));

    EXPECT_EQ(chaotic::ConvertTo<ns::StringEnum>("foo"), ns::StringEnum::kFoo);
    EXPECT_EQ(chaotic::ConvertTo<ns::StringEnum>("bar"), ns::StringEnum::kBar);
    EXPECT_EQ(chaotic::ConvertTo<ns::StringEnum>("some!thing"), ns::StringEnum::kSomeThing);

    EXPECT_EQ(Convert("foo", chaotic::convert::To<ns::StringEnum>{}), ns::StringEnum::kFoo);
    UEXPECT_THROW_MSG(
        Convert("zoo", chaotic::convert::To<ns::StringEnum>{}),
        std::runtime_error,
        "Invalid enum value (zoo) for type ::ns::StringEnum"
    );

    EXPECT_EQ(Parse("foo", formats::parse::To<ns::StringEnum>{}), ns::StringEnum::kFoo);
    UEXPECT_THROW_MSG(
        Parse("zoo", formats::parse::To<ns::StringEnum>{}),
        std::runtime_error,
        "Invalid enum value (zoo) for type ::ns::StringEnum"
    );

    EXPECT_EQ(std::size(ns::kStringEnumValues), 3);

    const ns::StringEnum values[] = {ns::StringEnum::kFoo, ns::StringEnum::kBar, ns::StringEnum::kSomeThing};
    std::size_t index = 0;
    for (const auto& value : ns::kStringEnumValues) {
        EXPECT_EQ(value, values[index]);
        ++index;
    }
}

TEST(Simple, StringEnumSax) {
    auto obj = CallSaxParser<ns::StringEnum>("\"foo\"");
    EXPECT_EQ(obj, ns::StringEnum::kFoo);

    EXPECT_EQ(TestDomSerializer(obj).As<std::string>(), "foo");
    EXPECT_EQ(TestWriteToStream(obj), TestDomSerializer(obj));

    UEXPECT_THROW_MSG(
        CallSaxParser<ns::StringEnum>("\"zoo\""),
        formats::json::parser::ParseError,
        "Parse error at pos 5, path '': Invalid enum value (zoo) for type ::ns::StringEnum, the "
        "latest token was \"zoo\""
    );
}

TEST(Simple, StringEnumPgInteraction) {
    auto str = ToString(ns::StringEnum::kFoo);
    static_assert(
        std::is_same_v<decltype(str), std::string>,
        "storages::postgres::io::Codegen requires ToString(enum) that returns a string"
    );
    EXPECT_EQ(str, "foo");

    auto result = Parse("foo", formats::parse::To<ns::StringEnum>{});
    static_assert(
        std::is_same_v<decltype(result), ns::StringEnum>,
        "storages::postgres::io::Codegen requires Parse(string_view, To<E>) that returns an enum E"
    );
    EXPECT_EQ(result, ns::StringEnum::kFoo);
}

TEST(Simple, HyphenField) {
    const ns::ObjectWithHyphenField obj;
    EXPECT_EQ(obj.foo_field, std::nullopt);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, RequiredNullableType) {
    ns::ObjectWithNullable obj;
    static_assert(std::is_same_v<decltype(obj.foo), std::optional<int>>);
}

TEST(Simple, RequiredNullableExists) {
    auto json = formats::json::MakeObject("foo", 1);
    auto obj = json.As<ns::ObjectWithNullable>();

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, RequiredNullableNull) {
    auto json = formats::json::MakeObject("foo", nullptr);
    auto obj = json.As<ns::ObjectWithNullable>();

    EXPECT_EQ(obj.foo, std::nullopt);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, RequiredNullableMissing) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::ObjectWithNullable>();

    EXPECT_EQ(obj.foo, std::nullopt);
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

USERVER_NAMESPACE_END
