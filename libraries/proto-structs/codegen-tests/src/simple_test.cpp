#include <type_traits>

#include <gtest/gtest.h>

#include <userver/proto-structs/convert.hpp>

#include <simple/file1.pb.h>
#include <simple/file1.structs.usrv.pb.hpp>

namespace ss = simple::structs;

USERVER_NAMESPACE_BEGIN

TEST(SingleFile, SimpleStruct) {
    static_assert(std::is_aggregate_v<ss::SimpleStruct>);
    [[maybe_unused]] ss::SimpleStruct message;
    message.some_integer = 5;
    message.some_text = std::optional<std::string>("foo");
    message.is_condition = true;
    message.some_bytes = {"foo", "bar"};
    message.something.set_bar("bar_val");
    message.inner_enum = ss::SimpleStruct::InnerEnum2::FOO_VAL;

    ss::SimpleStruct::ProtobufMessage vanilla;

    ::proto_structs::StructToMessage(std::move(message), vanilla);

    EXPECT_EQ(vanilla.some_integer(), 5);
    EXPECT_EQ(vanilla.some_text(), "foo");
    EXPECT_EQ(vanilla.is_condition(), true);
    EXPECT_EQ(vanilla.some_bytes().Get(0), "foo");
    EXPECT_EQ(vanilla.some_bytes().Get(1), "bar");
    EXPECT_EQ(vanilla.bar(), "bar_val");
    EXPECT_EQ(vanilla.inner_enum(), ss::SimpleStruct::ProtobufMessage::FOO_VAL);

    ss::SimpleStruct to;
    ::proto_structs::MessageToStruct(vanilla, to);

    ASSERT_EQ(to.some_integer, 5);
    ASSERT_EQ(to.some_text, std::optional<std::string>("foo"));
    ASSERT_TRUE(to.is_condition);
    std::vector<std::string> exp = {"foo", "bar"};
    ASSERT_EQ(to.some_bytes, exp);
    ASSERT_EQ(to.something.bar(), "bar_val");
    ASSERT_THROW([[maybe_unused]] auto foo = to.something.foo(), proto_structs::OneofAccessError);
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
    EXPECT_THROW([[maybe_unused]] const auto& not_found1 = none.foo(), proto_structs::OneofAccessError);
    EXPECT_THROW([[maybe_unused]] const auto& not_found2 = none.bar(), proto_structs::OneofAccessError);
}

TEST(Oneof, MakeFoo) {
    ss::SimpleStruct::Something foo;
    foo.set_foo(42);
    EXPECT_TRUE(foo);
    EXPECT_TRUE(foo.has_foo());
    EXPECT_NO_THROW(EXPECT_EQ(foo.foo(), 42));
    EXPECT_FALSE(foo.has_bar());
    EXPECT_THROW([[maybe_unused]] const auto& not_found = foo.bar(), proto_structs::OneofAccessError);
}

TEST(Oneof, MakeBar) {
    ss::SimpleStruct::Something bar;
    bar.set_bar("bar");
    EXPECT_TRUE(bar);
    EXPECT_FALSE(bar.has_foo());
    EXPECT_THROW([[maybe_unused]] const auto& not_found = bar.foo(), proto_structs::OneofAccessError);
    EXPECT_TRUE(bar.has_bar());
    EXPECT_NO_THROW(EXPECT_EQ(bar.bar(), "bar"));
}

TEST(Oneof, OneofInStruct) {
    [[maybe_unused]] ss::SimpleStruct message;
    message.something.set_bar("bar");
    EXPECT_EQ(message.something.bar(), "bar");
}

USERVER_NAMESPACE_END
