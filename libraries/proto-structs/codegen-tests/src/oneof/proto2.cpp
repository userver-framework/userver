#include <type_traits>

#include <gtest/gtest.h>

#include <oneof/proto2.structs.usrv.pb.hpp>

TEST(OneofProto2, OneofEmpty) {
    oneof::structs::Proto2 message;
    static_assert(std::is_same_v<decltype(message.oneof), oneof::structs::Proto2::Oneof>);

    EXPECT_FALSE(message.oneof.has_integer());
    EXPECT_THROW(message.oneof.integer(), std::bad_optional_access);
}

TEST(OneofProto2, OneofFundamentalTypes) {
    oneof::structs::Proto2 message;

    message.oneof.set_integer(42);
    EXPECT_TRUE(message.oneof.has_integer());
    EXPECT_EQ(message.oneof.integer(), 42);
    EXPECT_FALSE(message.oneof.has_string());
    EXPECT_THROW(message.oneof.string(), std::bad_variant_access);

    message.oneof.set_string("proto2_text");
    EXPECT_TRUE(message.oneof.has_string());
    EXPECT_EQ(message.oneof.string(), "proto2_text");
    EXPECT_FALSE(message.oneof.has_integer());
    EXPECT_THROW(message.oneof.integer(), std::bad_variant_access);
}

// TODO enable once fields of message and enum types are implemented.
#if 0

TEST(OneofProto2, OneofMessage) {
    oneof::structs::Proto2 message;

    message.oneof.set_message({.field = "message_text"});
    EXPECT_TRUE(message.oneof.has_message());
    EXPECT_EQ(message.oneof.message().field, "message_text");
}

TEST(OneofProto2, OneofEnum) {
    oneof::structs::Proto2 message;

    message.oneof.set_enum(oneof::structs::Proto2::ENUM2_FOO);
    EXPECT_TRUE(message.oneof.has_enum());
    EXPECT_EQ(message.oneof.enum_(), oneof::structs::Proto2::ENUM2_FOO);
}

TEST(OneofProto2, OneofGroup) {
    oneof::structs::Proto2 message;

    oneof::structs::Proto2::Group group;
    group.x = 100;
    group.y = "group_text";
    message.oneof.set_group(group);

    EXPECT_TRUE(message.oneof.has_group());
    EXPECT_EQ(message.oneof.group().x, 100);
    EXPECT_EQ(message.oneof.group().y, "group_text");
}

TEST(OneofProto2, GroupOneof) {
    oneof::structs::Proto2 message;

    oneof::structs::Proto2::Group group;
    group.x = 100;
    group.group_oneof.set_z({.field = "nested_message"});
    message.oneof.set_group(group);

    EXPECT_TRUE(message.oneof.has_group());
    EXPECT_TRUE(message.oneof.group().group_oneof.has_z());
    EXPECT_EQ(message.oneof.group().group_oneof.z().field, "nested_message");

    // Change the oneof inside the group
    message.oneof.group().group_oneof.set_w(oneof::structs::Proto2::ENUM2_FOO);
    EXPECT_TRUE(message.oneof.group().group_oneof.has_w());
    EXPECT_EQ(message.oneof.group().group_oneof.w(), oneof::structs::Proto2::ENUM2_FOO);
    EXPECT_FALSE(message.oneof.group().group_oneof.has_z());
}

TEST(OneofProto2, NestedTypesOutsideGroup) {
    oneof::structs::Proto2 message;

    // Using MessageInGroup outside the group context
    message.message_from_group.foo = "foo_value";
    EXPECT_EQ(message.message_from_group.foo, "foo_value");

    // Using EnumInGroup outside the group context
    message.enum_from_group.push_back(oneof::structs::Proto2::Group::ENUM_IN_GROUP_FOO);
    EXPECT_EQ(message.enum_from_group.size(), 1);
    EXPECT_EQ(message.enum_from_group[0], oneof::structs::Proto2::Group::ENUM_IN_GROUP_FOO);
}

#endif

TEST(OneofProto2, SingleFieldOneof) {
    oneof::structs::Proto2 message;
    static_assert(std::is_same_v<decltype(message.single_field_oneof), oneof::structs::Proto2::SingleFieldOneof>);

    EXPECT_FALSE(message.single_field_oneof.has_single());

    message.single_field_oneof.set_single("single_field_text");
    EXPECT_TRUE(message.single_field_oneof.has_single());
    EXPECT_EQ(message.single_field_oneof.single(), "single_field_text");
}

TEST(OneofProto2, OptionalFieldBefore) {
    oneof::structs::Proto2 message;
    static_assert(std::is_same_v<decltype(message.field_before), std::optional<std::string>>);

    EXPECT_FALSE(message.field_before.has_value());

    message.field_before = "field_before_value";
    EXPECT_TRUE(message.field_before.has_value());
    EXPECT_EQ(*message.field_before, "field_before_value");
}
