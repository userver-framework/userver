#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <schemas/indirect.hpp>
#include <schemas/indirect_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

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
    EXPECT_EQ(TestToJsonString(obj), json);
    EXPECT_EQ(TestWriteToStream(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

TEST(Simple, IndirectRefValidatorMinLength) {
    auto json = formats::json::MakeObject("data", "root", "left", formats::json::MakeObject("data", ""));
    UEXPECT_THROW_MSG(
        json.As<ns::TreeNode>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'left.data': Too short string, minimum length=1, given=0"
    );
}

TEST(Simple, IndirectRefValidatorMinLengthSax) {
    auto json = formats::json::MakeObject("data", "root", "left", formats::json::MakeObject("data", ""));
    UEXPECT_THROW_MSG(
        CallSaxParser<ns::TreeNode>(ToString(json)),
        formats::json::parser::ParseError,
        "Error at path 'left.data': Too short string, minimum length=1, given=0"
    );
}

TEST(Simple, IndirectRefValidatorMaxLength) {
    auto json = formats::json::MakeObject(
        "data",
        "root",
        "right",
        formats::json::MakeObject("data", "right", "left", formats::json::MakeObject("data", "this string is too long"))
    );
    UEXPECT_THROW_MSG(
        json.As<ns::TreeNode>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'right.left.data': Too long string, maximum length=20, given=23"
    );
}

TEST(Simple, IndirectRefValidatorMaxLengthSax) {
    auto json = formats::json::MakeObject(
        "data",
        "root",
        "right",
        formats::json::MakeObject("data", "right", "left", formats::json::MakeObject("data", "this string is too long"))
    );
    UEXPECT_THROW_MSG(
        CallSaxParser<ns::TreeNode>(ToString(json)),
        formats::json::parser::ParseError,
        "Error at path 'right.left.data': Too long string, maximum length=20, given=23"
    );
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

    auto obj = CallSaxParser<ns::TreeNode>(ToString(json));
    EXPECT_EQ(obj.data, "smth");
    EXPECT_EQ(obj.left, (ns::TreeNode{"left", std::nullopt, std::nullopt}));
    EXPECT_EQ(obj.right, (ns::TreeNode{"right", ns::TreeNode{"rightleft", std::nullopt, std::nullopt}, std::nullopt}));
    EXPECT_EQ(TestToJsonString(obj), json);

    EXPECT_EQ(TestDomSerializer(obj), json);
}

USERVER_NAMESPACE_END
