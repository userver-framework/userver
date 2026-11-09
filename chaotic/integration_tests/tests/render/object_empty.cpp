#include <initializer_list>
#include <string_view>

#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <schemas/object_empty.hpp>
#include <schemas/object_empty_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

static_assert(std::is_base_of_v<formats::json::Exception, chaotic::Error<formats::json::Value>>);
static_assert(std::is_base_of_v<formats::json::Exception, formats::json::parser::ParseError>);

TEST(Simple, Empty) {
    auto json = formats::json::MakeObject();
    auto obj = json.As<ns::ObjectEmpty>();
    EXPECT_EQ(obj, ns::ObjectEmpty());
    EXPECT_EQ(ToJsonString(obj), ToString(json));
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Simple, EmptySax) {
    auto json = formats::json::MakeObject();
    auto obj = CallSaxParser<ns::ObjectEmpty>(ToString(json));
    EXPECT_EQ(obj, ns::ObjectEmpty());
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Simple, ExtraOnlySax) {
    for (const std::string_view input : {
             "{}",
             R"({"null":null,"bool":true,"int":-42,"uint":18446744073709551615,
                 "double":1.25,"string":"text","array":[1,{"nested":[false,null]}]})",
         })
    {
        const auto json = formats::json::FromString(input);
        const auto obj = CallSaxParser<ns::ObjectExtraOnly>(input);
        EXPECT_EQ(obj.extra, json);
        EXPECT_EQ(TestWriteToStream(obj), json);
    }
}

TEST(Simple, ExtraOnlySaxInvalidAndReuse) {
    chaotic::sax::Parser<ns::ObjectExtraOnly> parser;
    for (const std::string_view input : {"null", "true", "42", R"("text")", "[]", "{", R"({"key":[1,})"}) {
        EXPECT_THROW((formats::json::parser::impl::ParseSingle(parser, input)), formats::json::parser::ParseError);
        const auto result = formats::json::parser::impl::ParseSingle(parser, R"({"ok":true})");
        EXPECT_EQ(result.extra, formats::json::FromString(R"({"ok":true})"));
    }
}

TEST(Simple, ExtraOnlySaxArray) {
    const auto objects = CallSaxParser<ns::ObjectExtraOnlyArray>(R"([{"first":[1,2]}, {}, {"second":false}])");
    ASSERT_EQ(objects.size(), 3);
    EXPECT_EQ(objects[0].extra, formats::json::FromString(R"({"first":[1,2]})"));
    EXPECT_EQ(objects[1].extra, formats::json::MakeObject());
    EXPECT_EQ(objects[2].extra, formats::json::FromString(R"({"second":false})"));
}

USERVER_NAMESPACE_END
