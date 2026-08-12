#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <gmock/gmock.h>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <schemas/oneofdiscriminator.hpp>
#include <schemas/oneofdiscriminator_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, OneOfDiscriminatorRefValidatorMinimum) {
    auto json = formats::json::MakeObject("foo", formats::json::MakeObject("type", "aaa", "a_prop", 0));
    UEXPECT_THROW_MSG(
        json.As<ns::OneOfDiscriminator>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'foo.a_prop': Invalid value, minimum=1, given=0"
    );
}

TEST(Simple, OneOfDiscriminatorRefValidatorMaximum) {
    auto json = formats::json::MakeObject("foo", formats::json::MakeObject("type", "bbb", "b_prop", 101));
    UEXPECT_THROW_MSG(
        json.As<ns::OneOfDiscriminator>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'foo.b_prop': Invalid value, maximum=100, given=101"
    );
}

TEST(Simple, RefToCValidatorMinimum) {
    auto json = formats::json::MakeObject("version", 30);
    UEXPECT_THROW_MSG(
        json.As<ns::RefToC>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'version': Invalid value, minimum=40, given=30"
    );
}

TEST(Simple, OneOfWithDiscriminator2) {
    auto json =
        formats::json::MakeObject("foo", formats::json::MakeObject("type", "aaa", "a_prop", 42, "additional", 14));
    auto obj = json.As<ns::OneOfDiscriminator>();
    EXPECT_EQ(std::get<0>(obj.foo.value()).type, "aaa");
    EXPECT_EQ(std::get<0>(obj.foo.value()).a_prop, 42);
    EXPECT_EQ(std::get<0>(obj.foo.value()).extra["additional"].As<int>(), 14);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_EQ(TestWriteToStream(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    EXPECT_EQ(TestDomSerializer(obj), json);

    std::get<0>(obj.foo.value()).type = "incorrect";
    EXPECT_EQ(std::get<0>(obj.foo.value()).type, "incorrect");
    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}

TEST(Simple, OneOfWithDiscriminator3) {
    auto json =
        formats::json::MakeObject("foo", formats::json::MakeObject("type", "bbb", "b_prop", 42, "additional", 14));
    auto obj = json.As<ns::OneOfDiscriminator>();
    EXPECT_EQ(std::get<1>(obj.foo.value()).type, "bbb");
    EXPECT_EQ(std::get<1>(obj.foo.value()).b_prop, 42);
    EXPECT_EQ(std::get<1>(obj.foo.value()).extra["additional"].As<int>(), 14);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";

    EXPECT_EQ(TestDomSerializer(obj), json);

    std::get<1>(obj.foo.value()).type = "incorrect";
    EXPECT_EQ(std::get<1>(obj.foo.value()).type, "incorrect");
    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"type\"")) << "No 'type'";
    EXPECT_EQ(ToJsonString(obj).find("\"type\""), ToJsonString(obj).rfind("\"type\"")) << "Multiple 'type's";
}

TEST(Simple, OneOfWithDiscriminator4) {
    auto json = formats::json::MakeObject("foo", formats::json::MakeObject("version", 42));
    auto obj = json.As<ns::IntegerOneOfDiscriminator>();
    EXPECT_EQ(std::get<0>(obj.foo.value()).version, 42);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"version\"")) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";

    EXPECT_EQ(TestDomSerializer(obj), json);

    std::get<0>(obj.foo.value()).version = 777;
    EXPECT_EQ(std::get<0>(obj.foo.value()).version, 777);
    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"version\"")) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";
}

TEST(Simple, OneOfWithDiscriminator5) {
    auto json = formats::json::MakeObject("foo", formats::json::MakeObject("version", 52));
    auto obj = json.As<ns::IntegerOneOfDiscriminator>();
    EXPECT_EQ(std::get<1>(obj.foo.value()).version, 52);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"version\"")) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";

    EXPECT_EQ(TestDomSerializer(obj), json);

    std::get<1>(obj.foo.value()).version = 777;
    EXPECT_EQ(std::get<1>(obj.foo.value()).version, 777);
    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_THAT(ToJsonString(obj), testing::HasSubstr("\"version\"")) << "No 'version'";
    EXPECT_EQ(ToJsonString(obj).find("\"version\""), ToJsonString(obj).rfind("\"version\"")) << "Multiple 'version's";
}

TEST(Simple, OneOfWithDiscriminator2Sax) {
    auto json =
        formats::json::MakeObject("foo", formats::json::MakeObject("type", "aaa", "a_prop", 42, "additional", 14));
    auto obj = CallSaxParser<ns::OneOfDiscriminator>(ToString(json));
    EXPECT_EQ(std::get<0>(obj.foo.value()).type, "aaa");
    EXPECT_EQ(std::get<0>(obj.foo.value()).a_prop, 42);
    EXPECT_EQ(std::get<0>(obj.foo.value()).extra["additional"].As<int>(), 14);
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Simple, OneOfWithDiscriminatorMapping) {
    EXPECT_EQ(ns::IntegerOneOfDiscriminator::kFoo_Settings.mapping.Describe(), "'42', '52'");
    EXPECT_EQ(ns::OneOfDiscriminator::kFoo_Settings.mapping.Describe(), "'aaa', 'bbb'");
}

USERVER_NAMESPACE_END
