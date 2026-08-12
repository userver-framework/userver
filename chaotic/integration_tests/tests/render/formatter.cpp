#include <userver/formats/json/inline.hpp>
#include <userver/utest/assert_macros.hpp>

#include <gmock/gmock.h>

#include <schemas/object_single_field.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Formatter, StringEnum) {
    auto json = formats::json::MakeObject("one", "foo", "two", "bar");
    auto obj1 = json["one"].As<ns::StringEnum>();
    auto obj2 = json["two"].As<ns::StringEnum>();
    const auto obj_str = fmt::format("we are {} and {}", obj1, obj2);
    EXPECT_EQ(obj_str, "we are foo and bar");
}

TEST(Formatter, IntegerEnum) {
    auto json = formats::json::MakeObject("one", 1, "two", 2);
    auto obj1 = json["one"].As<ns::IntegerEnum>();
    auto obj2 = json["two"].As<ns::IntegerEnum>();
    const auto obj_str = fmt::format("{0} + {1} = {0}{1}", obj1, obj2);
    EXPECT_EQ(obj_str, "1 + 2 = 12");
}

USERVER_NAMESPACE_END
