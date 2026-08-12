#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <schemas/object_keywords.hpp>
#include <schemas/object_keywords_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Simple, ObjectKeywords) {
    auto json = formats::json::MakeObject("int", 1, "delete", 2, "new", 3, "co_await", 4, "co_yield", 5, "class", 6);
    auto obj = json.As<ns::struct_>();

    EXPECT_EQ(obj.int_, 1);
    EXPECT_EQ(obj.delete_, 2);
    EXPECT_EQ(obj.new_, 3);
    EXPECT_EQ(obj.co_await_, 4);
    EXPECT_EQ(obj.co_yield_, 5);
    EXPECT_EQ(obj.class_, 6);

    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
    EXPECT_EQ(TestWriteToStream(obj), TestDomSerializer(obj));
}

TEST(Simple, ObjectKeywordsSax) {
    auto json = formats::json::MakeObject("int", 7, "delete", 8, "new", 9, "co_await", 10, "co_yield", 11, "class", 12);
    auto obj = CallSaxParser<ns::struct_>(ToString(json));

    EXPECT_EQ(obj.int_, 7);
    EXPECT_EQ(obj.delete_, 8);
    EXPECT_EQ(obj.new_, 9);
    EXPECT_EQ(obj.co_await_, 10);
    EXPECT_EQ(obj.co_yield_, 11);
    EXPECT_EQ(obj.class_, 12);

    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, ObjectBadNaming) {
    auto json = formats::json::MakeObject("some+ugly-name with%20trash", 42);
    auto obj = json.As<ns::Bad_naming>();

    EXPECT_EQ(obj.some_ugly_name_with_20trash, 42);

    EXPECT_EQ(TestDomSerializer(obj), json);
    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

TEST(Simple, ObjectBadNamingSax) {
    auto json = formats::json::MakeObject("some+ugly-name with%20trash", 42);
    auto obj = CallSaxParser<ns::Bad_naming>(ToString(json));

    EXPECT_EQ(obj.some_ugly_name_with_20trash, 42);

    EXPECT_EQ(TestToJsonString(obj), TestDomSerializer(obj));
}

USERVER_NAMESPACE_END
