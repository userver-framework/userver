#include <type_traits>

#include <gtest/gtest.h>

#include <userver/proto-structs/convert.hpp>
#include <userver/proto-structs/type_mapping.hpp>

#include <simple/base.pb.h>
#include <simple/base.structs.usrv.pb.hpp>

#include <google/protobuf/any.h>

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
    message.inner_enum = ss::SimpleStruct::InnerEnum2::kFooVal;
    message.nested.swag = "foo";
    message.optional_nested = std::optional<ss::SimpleStruct::NestedStruct>{{.swag = "foo"}};

    auto vanilla = ::proto_structs::StructToMessage(std::move(message));

    EXPECT_EQ(vanilla.some_integer(), 5);
    EXPECT_EQ(vanilla.some_text(), "foo");
    EXPECT_EQ(vanilla.is_condition(), true);
    EXPECT_EQ(vanilla.some_bytes().Get(0), "foo");
    EXPECT_EQ(vanilla.some_bytes().Get(1), "bar");
    EXPECT_EQ(vanilla.bar(), "bar_val");
    EXPECT_EQ(vanilla.inner_enum(), ss::SimpleStruct::ProtobufMessage::FOO_VAL);
    EXPECT_EQ(vanilla.nested().swag(), "foo");
    EXPECT_EQ(vanilla.optional_nested().swag(), "foo");

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
    [[maybe_unused]] const auto inner_enum1 = ss::SimpleStruct::NestedStruct::NestedStruct2::InnerEnum1::kBarVal;
}

TEST(SingleFile, InnerEnum2) {
    static_assert(std::is_enum_v<ss::SimpleStruct::InnerEnum2>);
    [[maybe_unused]] const auto inner_enum2 = ss::SimpleStruct::InnerEnum2::kFooVal;
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

TEST(Oneof, WellKnownTypes) {
    const std::chrono::seconds kSeconds{1};
    const std::chrono::nanoseconds kNanoseconds{1};
    const std::chrono::year kYear{2025};
    const std::chrono::month kMonth{10};
    const std::chrono::day kDay{30};
    const std::chrono::hours kHours{20};
    const std::chrono::minutes kMinutes{10};
    const std::string kString{"swag"};

    ss::WellKnownUsrv message;

    message.f1 = proto_structs::Timestamp(kSeconds, kNanoseconds);
    message.f2 = proto_structs::Duration(kSeconds, kNanoseconds);
    message.f3 = proto_structs::Date(kYear, kMonth, kDay);
    message.f4 = proto_structs::TimeOfDay(kHours, kMinutes, kSeconds);

    google::protobuf::Any pbuf_any;
    proto_structs::traits::CompatibleMessageType<ss::ForAny> for_any;
    for_any.set_f1(kString);

    EXPECT_TRUE(pbuf_any.PackFrom(for_any));

    message.f5 = proto_structs::Any{pbuf_any};

    const auto vanilla = proto_structs::StructToMessage(std::move(message));

    ss::WellKnownUsrv parsed;
    proto_structs::MessageToStruct(vanilla, parsed);

    ASSERT_EQ(parsed.f1.Seconds(), kSeconds);
    ASSERT_EQ(parsed.f1.Nanos(), kNanoseconds);

    ASSERT_EQ(parsed.f2.Seconds(), kSeconds);
    ASSERT_EQ(parsed.f2.Nanos(), kNanoseconds);

    ASSERT_TRUE(parsed.f3.Year().has_value());
    ASSERT_TRUE(parsed.f3.Month().has_value());
    ASSERT_TRUE(parsed.f3.Day().has_value());

    ASSERT_EQ(parsed.f3.Year(), kYear);
    ASSERT_EQ(parsed.f3.Month(), kMonth);
    ASSERT_EQ(parsed.f3.Day(), kDay);

    ASSERT_EQ(parsed.f4.Hours(), kHours);
    ASSERT_EQ(parsed.f4.Minutes(), kMinutes);
    ASSERT_EQ(parsed.f4.Seconds(), kSeconds);
    ASSERT_EQ(parsed.f4.Nanos(), std::chrono::nanoseconds{0});

    ASSERT_TRUE(parsed.f5.Is<proto_structs::traits::CompatibleMessageType<ss::ForAny>>());

    const auto parsed_any = parsed.f5.Unpack<ss::ForAny>();
    ASSERT_EQ(parsed_any.f1, kString);
}

USERVER_NAMESPACE_END
