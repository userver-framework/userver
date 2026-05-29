#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <boost/uuid/string_generator.hpp>

#include <userver/chaotic/array.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/variant.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/formats/yaml/value.hpp>
#include <userver/formats/yaml/value_builder.hpp>

#include <schemas/all_of.hpp>
#include <schemas/all_of_sax_parsers.hpp>
#include <schemas/date.hpp>
#include <schemas/extra_container.hpp>
#include <schemas/indirect.hpp>
#include <schemas/indirect_sax_parsers.hpp>
#include <schemas/object_empty.hpp>
#include <schemas/object_name.hpp>
#include <schemas/object_object.hpp>
#include <schemas/object_single_field.hpp>
#include <schemas/object_single_field_sax_parsers.hpp>
#include <schemas/one_of.hpp>
#include <schemas/one_of_sax_parsers.hpp>
#include <schemas/oneofdiscriminator.hpp>
#include <schemas/oneofdiscriminator_sax_parsers.hpp>
#include <schemas/string64.hpp>
#include <schemas/uri.hpp>
#include <schemas/uuid.hpp>
#include <userver/formats/json/string_builder.hpp>

USERVER_NAMESPACE_BEGIN

// Make sure that chaotic and SAX parser exceptions have common base
static_assert(std::is_base_of_v<formats::json::Exception, chaotic::Error<formats::json::Value>>);
static_assert(std::is_base_of_v<formats::json::Exception, formats::json::parser::ParseError>);

namespace {

template <class T>
std::string TestStreamJson(const T& value) {
    formats::json::StringBuilder builder;
    WriteToStream(value, builder);
    return builder.GetString();
}

}  // namespace

TEST(Simple, Empty) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::ObjectEmpty>();
    EXPECT_EQ(obj, ns::ObjectEmpty());
    EXPECT_EQ(ToJsonString(obj), ToString(json));
}

TEST(Simple, Integer) {
    auto json = formats::json::MakeObject("int3", 1, "integer", 3);
    auto obj = json.As<ns::SimpleObject>();
    EXPECT_EQ(obj.integer, 3);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());
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
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());
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
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

template <typename T>
T ParseToType(std::string_view input) {
    using Parser = chaotic::sax::Parser<T>;
    return formats::json::parser::ParseToType<T, Parser>(input);
}

TEST(Simple, ObjectWithAdditionalPropertiesInt) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3);
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesInt>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, (std::unordered_map<std::string, int>{{"two", 2}, {"three", 3}}));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json_back);
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

TEST(Simple, ObjectWithAdditionalPropertiesIntSax) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3);
    auto obj = ParseToType<ns::ObjectWithAdditionalPropertiesInt>(ToString(json));

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, (std::unordered_map<std::string, int>{{"two", 2}, {"three", 3}}));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, ObjectWithAdditionalPropertiesTrue) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesTrue>();

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, formats::json::MakeObject("two", 2, "three", 3, "object", formats::json::MakeObject()));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
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
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());
}

TEST(Simple, ObjectWithAdditionalPropertiesTrueSax) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = ParseToType<ns::ObjectWithAdditionalPropertiesTrue>(ToString(json));

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(obj.extra, formats::json::MakeObject("two", 2, "three", 3, "object", formats::json::MakeObject()));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, ObjectExtraMemberFalse) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = json.As<ns::ObjectWithAdditionalPropertiesTrueExtraMemberFalse>();

    EXPECT_EQ(obj.one, 1);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, formats::json::MakeObject("one", 1)) << ToString(json_back);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json_back);
}

TEST(Simple, ObjectExtraMemberFalseSax) {
    auto json = formats::json::MakeObject("one", 1, "two", 2, "three", 3, "object", formats::json::MakeObject());
    auto obj = ParseToType<ns::ObjectWithAdditionalPropertiesTrueExtraMemberFalse>(ToString(json));

    EXPECT_EQ(obj.one, 1);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());
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
        ParseToType<ns::ObjectWithAdditionalPropertiesFalseStrict>(ToString(json)),
        formats::json::parser::ParseError,
        "Parse error at pos 14, path 'bar': unknown field 'bar' for type "
        "'ns::ObjectWithAdditionalPropertiesFalseStrict', the latest token was ,\"bar\""
    );
}

TEST(Simple, IntegerEnum) {
    auto json = formats::json::MakeObject("one", 1);
    auto obj = json["one"].As<ns::IntegerEnum>();

    EXPECT_EQ(obj, ns::IntegerEnum::k1);
    EXPECT_EQ(formats::json::FromString(TestStreamJson(obj)), json["one"]);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json["one"]) << ToString(json_back);

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
    auto obj = ParseToType<ns::IntegerEnum>("1");
    EXPECT_EQ(obj, ns::IntegerEnum::k1);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back.As<int>(), 1) << ToString(json_back);
    EXPECT_EQ(formats::json::FromString(TestStreamJson(obj)), json_back);

    auto json2 = formats::json::MakeObject("one", 5);
    UEXPECT_THROW_MSG(
        ParseToType<ns::IntegerEnum>("5"),
        formats::json::parser::ParseError,
        "Parse error at pos 1, path '': Invalid enum value (5) for type ::ns::IntegerEnum, the latest token was 5"
    );
}

TEST(Simple, StringEnum) {
    auto json = formats::json::MakeObject("one", "foo");
    auto obj = json["one"].As<ns::StringEnum>();
    EXPECT_EQ(obj, ns::StringEnum::kFoo);
    EXPECT_EQ(formats::json::FromString(TestStreamJson(obj)), json["one"]);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json["one"]) << ToString(json_back);

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
    auto obj = ParseToType<ns::StringEnum>("\"foo\"");
    EXPECT_EQ(obj, ns::StringEnum::kFoo);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back.As<std::string>(), "foo") << ToString(json_back);
    EXPECT_EQ(formats::json::FromString(TestStreamJson(obj)), json_back);

    UEXPECT_THROW_MSG(
        ParseToType<ns::StringEnum>("\"zoo\""),
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

TEST(Simple, AllOf) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    auto obj = json.As<ns::AllOf>();
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.bar, 2);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, AllOfSax) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    auto obj = ParseToType<ns::AllOf>(ToString(json));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.bar, 2);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, OneOf) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::OneOf>();
    EXPECT_EQ(formats::json::FromString(TestStreamJson(obj)), json);

    EXPECT_EQ(std::get<ns::OneOf__O2>(obj), ns::OneOf__O2());

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, OneOfSax) {
    auto json = formats::json::MakeObject();
    auto obj = ParseToType<ns::OneOf>(ToString(json));
    EXPECT_EQ(formats::json::FromString(TestStreamJson(obj)), json);

    EXPECT_EQ(std::get<ns::OneOf__O2>(obj), ns::OneOf__O2());

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, OneOfWithDiscriminator) {
    auto json = formats::json::MakeObject("oneof", formats::json::MakeObject("type", "ObjectFoo", "foo", 42));
    auto obj = json.As<ns::ObjectOneOfWithDiscriminator>();
    EXPECT_EQ(std::get<0>(obj.oneof.value()).type, "ObjectFoo");
    EXPECT_EQ(std::get<0>(obj.oneof.value()).foo, 42);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);

    std::get<0>(obj.oneof.value()).type = "incorrect";
    EXPECT_EQ(std::get<0>(obj.oneof.value()).type, "incorrect");
    EXPECT_EQ(formats::json::ValueBuilder{obj}.ExtractValue(), json);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}

TEST(Simple, OneOfWithDiscriminator2) {
    auto json =
        formats::json::MakeObject("foo", formats::json::MakeObject("type", "aaa", "a_prop", 42, "additional", 14));
    auto obj = json.As<ns::OneOfDiscriminator>();
    EXPECT_EQ(std::get<0>(obj.foo.value()).type, "aaa");
    EXPECT_EQ(std::get<0>(obj.foo.value()).a_prop, 42);
    EXPECT_EQ(std::get<0>(obj.foo.value()).extra["additional"].As<int>(), 14);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);

    std::get<0>(obj.foo.value()).type = "incorrect";
    EXPECT_EQ(std::get<0>(obj.foo.value()).type, "incorrect");
    EXPECT_EQ(formats::json::ValueBuilder{obj}.ExtractValue(), json);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}

TEST(Simple, OneOfWithDiscriminator3) {
    auto json =
        formats::json::MakeObject("foo", formats::json::MakeObject("type", "bbb", "b_prop", 42, "additional", 14));
    auto obj = json.As<ns::OneOfDiscriminator>();
    EXPECT_EQ(std::get<1>(obj.foo.value()).type, "bbb");
    EXPECT_EQ(std::get<1>(obj.foo.value()).b_prop, 42);
    EXPECT_EQ(std::get<1>(obj.foo.value()).extra["additional"].As<int>(), 14);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);

    std::get<1>(obj.foo.value()).type = "incorrect";
    EXPECT_EQ(std::get<1>(obj.foo.value()).type, "incorrect");
    EXPECT_EQ(formats::json::ValueBuilder{obj}.ExtractValue(), json);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}

TEST(Simple, OneOfWithDiscriminator4) {
    auto json = formats::json::MakeObject("foo", formats::json::MakeObject("version", 42));
    auto obj = json.As<ns::IntegerOneOfDiscriminator>();
    EXPECT_EQ(std::get<0>(obj.foo.value()).version, 42);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"version\""), std::string::npos) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);

    std::get<0>(obj.foo.value()).version = 777;
    EXPECT_EQ(std::get<0>(obj.foo.value()).version, 777);
    EXPECT_EQ(formats::json::ValueBuilder{obj}.ExtractValue(), json);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"version\""), std::string::npos) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";
}

TEST(Simple, OneOfWithDiscriminator5) {
    auto json = formats::json::MakeObject("foo", formats::json::MakeObject("version", 52));
    auto obj = json.As<ns::IntegerOneOfDiscriminator>();
    EXPECT_EQ(std::get<1>(obj.foo.value()).version, 52);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"version\""), std::string::npos) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);

    std::get<1>(obj.foo.value()).version = 777;
    EXPECT_EQ(std::get<1>(obj.foo.value()).version, 777);
    EXPECT_EQ(formats::json::ValueBuilder{obj}.ExtractValue(), json);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"version\""), std::string::npos) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";
}

TEST(Simple, OneOfWithDiscriminatorSax) {
    auto json = formats::json::MakeObject("oneof", formats::json::MakeObject("type", "ObjectFoo", "foo", 42));
    auto obj = ParseToType<ns::ObjectOneOfWithDiscriminator>(ToString(json));
    EXPECT_EQ(std::get<0>(obj.oneof.value()).type, "ObjectFoo");
    EXPECT_EQ(std::get<0>(obj.oneof.value()).foo, 42);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);
}

TEST(Simple, OneOfWithDiscriminatorMapping) {
    EXPECT_EQ(ns::IntegerOneOfDiscriminator::kFoo_Settings.mapping.Describe(), "'42', '52'");
    EXPECT_EQ(ns::OneOfDiscriminator::kFoo_Settings.mapping.Describe(), "'aaa', 'bbb'");
}

/* TODO:
TEST(Simple, AllOfOneOf) {
    auto json = formats::json::MakeObject("value", formats::json::MakeObject("foo", 1, "type", "B", "integer", 42));
    auto obj = json.As<ns::AllOfOneOf>();
    EXPECT_EQ(formats::json::FromString(TestStreamJson(obj)), json);

    EXPECT_EQ(obj.value.foo, 1);

    ns::OneOfDiscriminatorForAllOf& oneof = obj.value;
    EXPECT_EQ(std::get<1>(oneof).type, "B");
    EXPECT_EQ(std::get<1>(oneof).integer.value(), 42);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json_back, json) << ToString(json_back);

    std::get<1>(oneof).type = "broken";
    EXPECT_EQ(std::get<1>(oneof).type, "broken");
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);
    EXPECT_NE(ToJsonString(obj).find("\"type\""), std::string::npos) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}
*/

TEST(Simple, Indirect) {
    auto json = formats::json::MakeObject(
        "data",
        "smth",
        "left",
        formats::json::MakeObject("data", "left"),
        "right",
        formats::json::MakeObject("data", "right", "left", formats::json::MakeObject("data", "rightleft"))
    );

    auto obj = json.As<ns::TreeNode>();
    EXPECT_EQ(obj.data, "smth");
    EXPECT_EQ(obj.left, (ns::TreeNode{"left", std::nullopt, std::nullopt}));
    EXPECT_EQ(obj.right, (ns::TreeNode{"right", ns::TreeNode{"rightleft", std::nullopt, std::nullopt}, std::nullopt}));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, IndirectSax) {
    auto json = formats::json::MakeObject(
        "data",
        "smth",
        "left",
        formats::json::MakeObject("data", "left"),
        "right",
        formats::json::MakeObject("data", "right", "left", formats::json::MakeObject("data", "rightleft"))
    );

    auto obj = ParseToType<ns::TreeNode>(ToString(json));
    EXPECT_EQ(obj.data, "smth");
    EXPECT_EQ(obj.left, (ns::TreeNode{"left", std::nullopt, std::nullopt}));
    EXPECT_EQ(obj.right, (ns::TreeNode{"right", ns::TreeNode{"rightleft", std::nullopt, std::nullopt}, std::nullopt}));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, HyphenField) {
    const ns::ObjectWithHyphenField obj;
    EXPECT_EQ(obj.foo_field, std::nullopt);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());
}

TEST(Simple, SubSubObjectSmoke) { [[maybe_unused]] const ns::Objectx::Objectx_::Objectx__ x; }

TEST(Simple, ExtraType) {
    static_assert(std::is_same_v<
                  decltype(std::declval<ns::ObjectWithExtraType>().extra),
                  std::map<std::string, std::string>>);
}

TEST(Simple, CppName) {
    const ns::ObjectName obj;
    EXPECT_EQ(obj.bar, std::nullopt);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());
}

TEST(Simple, Date) {
    auto json = formats::json::MakeObject("created_at", "2020-10-01");
    auto obj = json.As<ns::ObjectDate>();
    EXPECT_EQ(obj.created_at, utils::datetime::Date(2020, 10, 01));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, DateTime) {
    auto date = "2020-10-01T12:34:56+12:34";
    auto json = formats::json::MakeObject("updated_at", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTz
        tp{utils::datetime::UtcStringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.updated_at, tp);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["updated_at"].As<std::string>();
    EXPECT_EQ(str, date);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, DateTimeExtra) {
    auto date = "2020-10-01T12:34:56+12:34";
    auto json = formats::json::MakeObject("updated_at_extra", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTz
        tp{utils::datetime::UtcStringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.updated_at_extra->time_point, tp);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["updated_at_extra"].As<std::string>();
    EXPECT_EQ(str, date);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, DateTimeIsoBasic) {
    auto date = "2020-10-01T12:34:56+1234";
    auto json = formats::json::MakeObject("deleted_at", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTzIsoBasic
        tp{utils::datetime::UtcStringtime("2020-10-01T00:00:56Z"), std::chrono::seconds(12 * 60 * 60 + 34 * 60)};
    EXPECT_EQ(obj.deleted_at, tp);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["deleted_at"].As<std::string>();
    EXPECT_EQ(str, date);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, DateTimeFraction) {
    auto date = "2020-10-01T12:34:56.789+0000";
    auto json = formats::json::MakeObject("modified_at", date);
    auto obj = json.As<ns::ObjectDate>();

    const utils::datetime::TimePointTz
        tp{utils::datetime::UtcStringtime("2020-10-01T12:34:56Z"), std::chrono::seconds(0)};
    EXPECT_EQ(obj.modified_at->GetTimePoint() - tp.GetTimePoint(), std::chrono::milliseconds(789));
    EXPECT_EQ(obj.modified_at->GetTzOffset(), std::chrono::seconds(0));
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["modified_at"].As<std::string>();
    EXPECT_EQ(str, date) << str;

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, Uuid) {
    auto uuid = "01234567-89ab-cdef-0123-456789abcdef";
    auto json = formats::json::MakeObject("uuid", uuid);
    auto obj = json.As<ns::ObjectUuid>();

    const boost::uuids::string_generator gen;
    const boost::uuids::uuid expected = gen(uuid);
    EXPECT_EQ(obj.uuid, expected);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["uuid"].As<std::string>();
    EXPECT_EQ(str, uuid);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, Uri) {
    auto uri = "http://example.com";
    auto json = formats::json::MakeObject("uri", uri);
    auto obj = json.As<ns::ObjectUri>();

    EXPECT_EQ(obj.uri, uri);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["uri"].As<std::string>();
    EXPECT_EQ(str, uri);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, String64) {
    auto str64 = crypto::base64::String64{"hello, userver!"};
    auto obj = ns::ObjectString64{str64};

    auto str = Serialize(obj, formats::serialize::To<formats::json::Value>())["value"].As<std::string>();
    EXPECT_EQ(str, "aGVsbG8sIHVzZXJ2ZXIh");
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());

    auto new_obj = formats::json::MakeObject("value", str).As<ns::ObjectString64>();
    EXPECT_EQ(new_obj.value, str64);
}

TEST(Simple, RequiredNullableType) {
    ns::ObjectWithNullable obj;
    static_assert(std::is_same_v<decltype(obj.foo), std::optional<int>>);
}

TEST(Simple, RequiredNullableExists) {
    auto json = formats::json::MakeObject("foo", 1);
    auto obj = json.As<ns::ObjectWithNullable>();

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

TEST(Simple, RequiredNullableNull) {
    auto json = formats::json::MakeObject("foo", nullptr);
    auto obj = json.As<ns::ObjectWithNullable>();

    EXPECT_EQ(obj.foo, std::nullopt);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), formats::json::ValueBuilder{obj}.ExtractValue());
}

TEST(Simple, RequiredNullableMissing) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::ObjectWithNullable>();

    EXPECT_EQ(obj.foo, std::nullopt);
    EXPECT_EQ(formats::json::FromString(ToJsonString(obj)), json);

    auto json_back = formats::json::ValueBuilder{obj}.ExtractValue();
    EXPECT_EQ(json, json_back) << ToString(json_back);
}

USERVER_NAMESPACE_END
