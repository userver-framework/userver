#include <type_traits>

#include <gtest/gtest.h>

#include <simple/file1.structs.usrv.pb.hpp>

namespace ss = simple::structs;

TEST(SingleFile, SimpleStruct) {
    static_assert(std::is_aggregate_v<ss::SimpleStruct>);
    [[maybe_unused]] ss::SimpleStruct message;
    message.some_integer = 5;
    message.some_text = std::optional<std::string>("foo");
    message.is_condition = true;
    message.some_bytes = {"foo", "bar"};
}

TEST(SingleFile, NestedStruct) {
    static_assert(std::is_aggregate_v<ss::SimpleStruct::NestedStruct>);
    [[maybe_unused]] ss::SimpleStruct::NestedStruct nested;
    nested.swag = "foo";

    static_assert(std::is_aggregate_v<ss::SimpleStruct::NestedStruct::NestedStruct2>);
    [[maybe_unused]] ss::SimpleStruct::NestedStruct::NestedStruct2 nested2;
    nested2.swag2 = "bar";
}

TEST(SingleFile, InnerEnum1) {
    static_assert(std::is_enum_v<ss::SimpleStruct::NestedStruct::NestedStruct2::InnerEnum1>);
    [[maybe_unused]] const auto inner_enum1 = ss::SimpleStruct::NestedStruct::NestedStruct2::InnerEnum1::BAR_VAL;
}

TEST(SingleFile, InnerEnum2) {
    static_assert(std::is_enum_v<ss::SimpleStruct::InnerEnum2>);
    [[maybe_unused]] const auto inner_enum2 = ss::SimpleStruct::InnerEnum2::FOO_VAL;
}

TEST(SingleFile, SecondStruct) {
    static_assert(std::is_aggregate_v<ss::SecondStruct>);
    [[maybe_unused]] ss::SecondStruct message;
}

TEST(SingleFile, GlobalEnum) {
    static_assert(std::is_enum_v<ss::GlobalEnum>);
    [[maybe_unused]] ss::GlobalEnum message{};
}

TEST(Oneof, Empty) {
    const ss::SimpleStruct::Something none;
    EXPECT_FALSE(none);
    EXPECT_FALSE(none.has_foo());
    EXPECT_FALSE(none.has_bar());
    EXPECT_THROW(none.foo(), std::bad_optional_access);
    EXPECT_THROW(none.bar(), std::bad_optional_access);
}

TEST(Oneof, MakeFoo) {
    ss::SimpleStruct::Something foo;
    foo.set_foo(42);
    EXPECT_TRUE(foo);
    EXPECT_TRUE(foo.has_foo());
    EXPECT_NO_THROW(EXPECT_EQ(foo.foo(), 42));
    EXPECT_FALSE(foo.has_bar());
    EXPECT_THROW(foo.bar(), std::bad_variant_access);
}

TEST(Oneof, MakeBar) {
    ss::SimpleStruct::Something bar;
    bar.set_bar("bar");
    EXPECT_TRUE(bar);
    EXPECT_FALSE(bar.has_foo());
    EXPECT_THROW(bar.foo(), std::bad_variant_access);
    EXPECT_TRUE(bar.has_bar());
    EXPECT_NO_THROW(EXPECT_EQ(bar.bar(), "bar"));
}

TEST(Oneof, OneofInStruct) {
    [[maybe_unused]] ss::SimpleStruct message;
    message.something.set_bar("bar");
    EXPECT_EQ(message.something.bar(), "bar");
}
