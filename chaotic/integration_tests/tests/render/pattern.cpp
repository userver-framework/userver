#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <userver/chaotic/exception.hpp>

#include <schemas/pattern.hpp>
#include <schemas/pattern_sax_parsers.hpp>

#include "helper.hpp"

USERVER_NAMESPACE_BEGIN

TEST(Pattern, FooValid) {
    auto json = formats::json::MakeObject("foo", "foobar");
    auto obj = json.As<ns::ObjectPattern>();
    EXPECT_EQ(obj.foo, "foobar");
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Pattern, ObjectPatternSax) {
    auto json = formats::json::MakeObject("foo", "foobar", "bar", "barbaz");
    auto obj = CallSaxParser<ns::ObjectPattern>(ToString(json));
    EXPECT_EQ(obj.foo, "foobar");
    EXPECT_EQ(obj.bar, "barbaz");
    EXPECT_EQ(TestWriteToStream(obj), json);
}

TEST(Pattern, FooInvalid) {
    auto json = formats::json::MakeObject("foo", "bar");
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectPattern>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'foo': doesn't match regex"
    );
}

TEST(Pattern, BarInvalid) {
    auto json = formats::json::MakeObject("bar", "foo");
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectPattern>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'bar': doesn't match regex"
    );
}

TEST(Pattern, ZooInvalid) {
    auto json = formats::json::MakeObject("zoo", "invalid");
    UEXPECT_THROW_MSG(
        json.As<ns::ObjectPattern>(),
        chaotic::Error<formats::json::Value>,
        "Error at path 'zoo': doesn't match regex"
    );
}

USERVER_NAMESPACE_END
