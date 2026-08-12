#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <gmock/gmock.h>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <schemas/all_of.hpp>
#include <schemas/all_of_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, AllOf) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    auto obj = json.As<ns::AllOf>();
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.bar, 2);
    EXPECT_TRUE(static_cast<ns::Object1&>(obj).extra.IsEmpty()) << ToString(static_cast<ns::Object1&>(obj).extra);
    EXPECT_TRUE(static_cast<ns::AllOf__P1&>(obj).extra.IsEmpty()) << ToString(static_cast<ns::AllOf__P1&>(obj).extra);
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, AllOfSax) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2);
    auto obj = CallSaxParser<ns::AllOf>(ToString(json));
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.bar, 2);
    EXPECT_TRUE(static_cast<ns::Object1&>(obj).extra.IsEmpty()) << ToString(static_cast<ns::Object1&>(obj).extra);
    EXPECT_TRUE(static_cast<ns::AllOf__P1&>(obj).extra.IsEmpty()) << ToString(static_cast<ns::AllOf__P1&>(obj).extra);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, AllOfAndAdditional) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2, "additional", "value", "additional1", "value1");
    auto obj = json.As<ns::AllOf>();
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.bar, 2);

    const auto& obj1 = static_cast<ns::Object1&>(obj);
    EXPECT_EQ(obj1.extra.GetSize(), 2) << ToString(obj1.extra);
    EXPECT_EQ(obj1.extra["additional"].As<std::string>(), "value") << ToString(obj1.extra);
    EXPECT_EQ(obj1.extra["additional1"].As<std::string>(), "value1") << ToString(obj1.extra);

    const auto& obj2 = static_cast<ns::AllOf__P1&>(obj);
    EXPECT_EQ(obj2.extra.GetSize(), 2) << ToString(obj2.extra);
    EXPECT_EQ(obj2.extra["additional"].As<std::string>(), "value") << ToString(obj2.extra);
    EXPECT_EQ(obj2.extra["additional1"].As<std::string>(), "value1") << ToString(obj2.extra);

    EXPECT_EQ(obj1.extra, obj2.extra);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, AllOfAndAdditionalSax) {
    auto json = formats::json::MakeObject("foo", 1, "bar", 2, "additional", "value", "additional1", "value1");
    auto obj = CallSaxParser<ns::AllOf>(ToString(json));
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.bar, 2);

    const auto& obj1 = static_cast<ns::Object1&>(obj);
    EXPECT_EQ(obj1.extra.GetSize(), 2) << ToString(obj1.extra);
    EXPECT_EQ(obj1.extra["additional"].As<std::string>(), "value") << ToString(obj1.extra);
    EXPECT_EQ(obj1.extra["additional1"].As<std::string>(), "value1") << ToString(obj1.extra);

    const auto& obj2 = static_cast<ns::AllOf__P1&>(obj);
    EXPECT_EQ(obj2.extra.GetSize(), 2) << ToString(obj2.extra);
    EXPECT_EQ(obj2.extra["additional"].As<std::string>(), "value") << ToString(obj2.extra);
    EXPECT_EQ(obj2.extra["additional1"].As<std::string>(), "value1") << ToString(obj2.extra);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, AllOfNoAdditional) {
    auto json = formats::json::MakeObject("foo", 1, "type", "2", "integer", 3);
    auto obj = json.As<ns::AllOfNoAdditional>();
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.type, "2");
    EXPECT_EQ(obj.integer, 3);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, AllOfRefValidatorMinimum) {
    auto json = formats::json::MakeObject("foo", 0, "bar", 2);
    UEXPECT_THROW_MSG(
        json.As<ns::AllOf>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'foo': Invalid value, minimum=1, given=0"
    );
}

TEST(Simple, AllOfRefValidatorMinimumSax) {
    auto json = formats::json::MakeObject("foo", 0, "bar", 2);
    UEXPECT_THROW_MSG(
        CallSaxParser<ns::AllOf>(ToString(json)),
        formats::json::parser::ParseError,
        "Error at path 'foo': Invalid value, minimum=1, given=0"
    );
}

TEST(Simple, AllOfNoAdditionalRefValidatorMaximum) {
    auto json = formats::json::MakeObject("foo", 1, "type", "B", "integer", 11);
    UEXPECT_THROW_MSG(
        json.As<ns::AllOfNoAdditional>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'integer': Invalid value, maximum=10, given=11"
    );
}

TEST(Simple, AllOfNoAdditionalRefValidatorMaximumSax) {
    auto json = formats::json::MakeObject("foo", 1, "type", "B", "integer", 11);
    UEXPECT_THROW_MSG(
        CallSaxParser<ns::AllOfNoAdditional>(ToString(json)),
        formats::json::parser::ParseError,
        "Error at path 'integer': Invalid value, maximum=10, given=11"
    );
}

TEST(Simple, AllOfNoAdditionalSax) {
    auto json = formats::json::MakeObject("foo", 1, "type", "2", "integer", 3);
    auto obj = CallSaxParser<ns::AllOfNoAdditional>(ToString(json));
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(obj.foo, 1);
    EXPECT_EQ(obj.type, "2");
    EXPECT_EQ(obj.integer, 3);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

/* TODO:
TEST(Simple, AllOfOneOf) {
    auto json = formats::json::MakeObject("value", formats::json::MakeObject("foo", 1, "type", "B", "integer", 42));
    auto obj = json.As<ns::AllOfOneOf>();
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(obj.value.foo, 1);
    EXPECT_TRUE(obj.value.extra.IsEmpty()) << ToString(obj.value.extra);

    ns::OneOfDiscriminatorForAllOf& oneof = obj.value;
    EXPECT_EQ(std::get<1>(oneof).type, "B");
    EXPECT_EQ(std::get<1>(oneof).integer.value(), 42);

    EXPECT_EQ(TestDomSerializer(obj), json);

    std::get<1>(oneof).type = "broken";
    EXPECT_EQ(std::get<1>(oneof).type, "broken");
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}*/

USERVER_NAMESPACE_END
