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

USERVER_NAMESPACE_END
