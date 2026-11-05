#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <gmock/gmock.h>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/may_parse.hpp>

#include <schemas/one_of.hpp>
#include <schemas/one_of_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, OneOf) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::OneOf>();
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(std::get<ns::OneOf__O2>(obj), ns::OneOf__O2());

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, OneOfSax) {
    auto json = formats::json::MakeObject();
    auto obj = CallSaxParser<ns::OneOf>(ToString(json));
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(std::get<ns::OneOf__O2>(obj), ns::OneOf__O2());

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, OneOfNumber) {
    auto json = formats::json::ValueBuilder(0.1).ExtractValue();
    auto obj = json.As<ns::OneOf>();
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(std::get<double>(obj), 0.1);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, OneOfNumberSax) {
    auto json = formats::json::ValueBuilder(0.1).ExtractValue();
    auto obj = CallSaxParser<ns::OneOf>(ToString(json));
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(std::get<double>(obj), 0.1);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, OneOfRefValidatorMinimum) {
    auto json = formats::json::MakeObject("oneof", 0.5);
    static_assert(std::is_base_of_v<formats::json::Exception, formats::json::ParseException>);
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectOneOfNoDiscriminator>(),
        formats::json::ParseException,
        "Error at path '/': Invalid value, minimum=1, given=0.5"
    );
}

TEST(Simple, OneOfRefValidatorMinimumSax) {
    UEXPECT_THROW_MSG(
        CallSaxParser<ns::ObjectOneOfNoDiscriminator>(R"({"oneof":0.1})"),
        formats::json::parser::ParseError,
        "Invalid value, minimum=1, given=0.1"
    );
}

TEST(Simple, OneOfWithDiscriminatorRefValidatorMinimum) {
    auto json = formats::json::MakeObject("oneof", formats::json::MakeObject("type", "ObjectFoo", "foo", 0));
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectOneOfWithDiscriminator>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'oneof.foo': Invalid value, minimum=1, given=0"
    );
}

TEST(Simple, OneOfWithDiscriminatorRefValidatorMinLength) {
    auto json = formats::json::MakeObject("oneof", formats::json::MakeObject("type", "ObjectBar", "bar", "ab"));
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectOneOfWithDiscriminator>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'oneof.bar': Too short string, minimum length=3, given=2"
    );
}

TEST(Simple, OneOfWithDiscriminator) {
    auto json = formats::json::MakeObject("oneof", formats::json::MakeObject("type", "ObjectFoo", "foo", 42));
    auto obj = json.As<ns::ObjectOneOfWithDiscriminator>();
    EXPECT_EQ(std::get<0>(obj.oneof.value()).type, "ObjectFoo");
    EXPECT_EQ(std::get<0>(obj.oneof.value()).foo, 42);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_EQ(TestWriteToStream(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    EXPECT_EQ(TestDomSerializer(obj), json);

    std::get<0>(obj.oneof.value()).type = "incorrect";
    EXPECT_EQ(std::get<0>(obj.oneof.value()).type, "incorrect");
    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}

TEST(Simple, OneOfWithDiscriminatorSax) {
    auto json = formats::json::MakeObject("oneof", formats::json::MakeObject("type", "ObjectFoo", "foo", 42));
    auto obj = CallSaxParser<ns::ObjectOneOfWithDiscriminator>(ToString(json));
    EXPECT_EQ(std::get<0>(obj.oneof.value()).type, "ObjectFoo");
    EXPECT_EQ(std::get<0>(obj.oneof.value()).foo, 42);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, MayParseAsObject) {
    EXPECT_TRUE(formats::parse::MayParseAs<ns::ObjectRequired>(formats::json::MakeObject("id", 1)));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::ObjectRequired>(formats::json::MakeObject("name", "x")));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::ObjectRequired>(formats::json::MakeObject("id", 1, "unknown", 1)));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::ObjectRequired>(formats::json::ValueBuilder(1).ExtractValue()));
}

TEST(Simple, MayParseAsObjectWithEnumField) {
    EXPECT_TRUE(formats::parse::MayParseAs<ns::ObjectWithKind>(formats::json::MakeObject("kind", "first")));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::ObjectWithKind>(formats::json::MakeObject("kind", "third")));

    const auto with_optional_kind = [](int optional_kind) {
        return formats::json::MakeObject("kind", "first", "optional_kind", optional_kind);
    };
    EXPECT_TRUE(formats::parse::MayParseAs<ns::ObjectWithKind>(with_optional_kind(2)));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::ObjectWithKind>(with_optional_kind(3)));
}

TEST(Simple, MayParseAsEnum) {
    EXPECT_TRUE(formats::parse::MayParseAs<ns::StringKind>(formats::json::ValueBuilder("first").ExtractValue()));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::StringKind>(formats::json::ValueBuilder("third").ExtractValue()));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::StringKind>(formats::json::ValueBuilder(1).ExtractValue()));

    EXPECT_TRUE(formats::parse::MayParseAs<ns::IntKind>(formats::json::ValueBuilder(1).ExtractValue()));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::IntKind>(formats::json::ValueBuilder(3).ExtractValue()));
    EXPECT_TRUE(formats::parse::MayParseAs<ns::IntKind>(formats::json::ValueBuilder(2.0).ExtractValue()));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::IntKind>(formats::json::ValueBuilder(1.5).ExtractValue()));
    EXPECT_FALSE(formats::parse::MayParseAs<ns::IntKind>(formats::json::ValueBuilder("first").ExtractValue()));
}

TEST(Simple, OneOfPrechecked) {
    const auto parse = [](const formats::json::Value& oneof) {
        return formats::json::MakeObject("oneof", oneof).As<ns::ObjectOneOfPrechecked>().oneof.value();
    };

    EXPECT_TRUE(std::holds_alternative<ns::ObjectRequired>(parse(formats::json::MakeObject("id", 1))));
    EXPECT_TRUE(std::holds_alternative<ns::ObjectFoo>(parse(formats::json::MakeObject("foo", 2))));
    EXPECT_TRUE(std::holds_alternative<ns::StringKind>(parse(formats::json::ValueBuilder("second").ExtractValue())));
    EXPECT_TRUE(std::holds_alternative<ns::IntKind>(parse(formats::json::ValueBuilder(2).ExtractValue())));
    UEXPECT_THROW_MSG(
        parse(formats::json::MakeObject("bar", 1)),
        formats::json::ParseException,
        "Unknown property 'bar'"
    );
}

USERVER_NAMESPACE_END
