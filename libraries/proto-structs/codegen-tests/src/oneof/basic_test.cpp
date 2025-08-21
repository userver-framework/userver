#include <type_traits>

#include <gtest/gtest.h>

#include <oneof/basic.structs.usrv.pb.hpp>

USERVER_NAMESPACE_BEGIN

TEST(OneofBasic, LowercaseEmpty) {
    oneof::structs::Parent message;
    static_assert(std::is_same_v<decltype(message.lowercase), oneof::structs::Parent::Lowercase>);

    EXPECT_FALSE(message.lowercase.has_integer());
    EXPECT_THROW(message.lowercase.integer(), std::bad_optional_access);
}

TEST(OneofBasic, LowercaseFundamentalTypes) {
    oneof::structs::Parent message;

    message.lowercase.set_integer(10);
    EXPECT_TRUE(message.lowercase.has_integer());
    EXPECT_EQ(message.lowercase.integer(), 10);
    EXPECT_FALSE(message.lowercase.has_string());
    EXPECT_THROW(message.lowercase.string(), std::bad_variant_access);

    message.lowercase.set_string("text");
    EXPECT_TRUE(message.lowercase.has_string());
    EXPECT_EQ(message.lowercase.string(), "text");
    EXPECT_FALSE(message.lowercase.has_integer());
    EXPECT_THROW(message.lowercase.integer(), std::bad_variant_access);
}

// TODO enable once fields of message and enum types are implemented.
#if 0

TEST(OneofBasic, LowercaseMessage) {
    oneof::structs::Parent message;

    message.lowercase.set_message1(oneof::structs::Message1{.field = "text"});
    EXPECT_TRUE(message.lowercase.has_message1());
    EXPECT_EQ(message.lowecase.message1().field, "text");

    message.lowercase.set_message2({.field = "text"});
    EXPECT_TRUE(message.lowercase.has_message2());
    EXPECT_EQ(message.lowercase.message2().field, "text");
}

TEST(OneofBasic, LowercaseEnum) {
    oneof::structs::Parent message;

    message.lowercase.set_enum1(oneof::structs::Enum1::ENUM1_FOO);
    EXPECT_TRUE(message.lowercase.has_enum1());
    EXPECT_EQ(message.lowercase.enum1(), oneof::structs::Enum1::ENUM1_FOO);

    message.lowercase.set_enum2(oneof::structs::Parent::Enum2::ENUM2_FOO);
    EXPECT_TRUE(message.lowercase.has_enum2());
    EXPECT_EQ(message.lowercase.enum2(), oneof::structs::Parent::Enum2::ENUM2_FOO);
}

#endif

TEST(OneofBasic, Uppercase) {
    oneof::structs::Parent message;
    static_assert(std::is_same_v<decltype(message.Uppercase), oneof::structs::Parent::TUppercase>);

    message.Uppercase.set_foo("text");
    EXPECT_TRUE(message.Uppercase.has_foo());
    EXPECT_EQ(message.Uppercase.foo(), "text");

    message.Uppercase.set_bar(10);
    EXPECT_TRUE(message.Uppercase.has_bar());
    EXPECT_EQ(message.Uppercase.bar(), 10);
}

TEST(OneofBasic, SingleFieldOneof) {
    oneof::structs::Parent message;
    static_assert(std::is_same_v<decltype(message.single_field_oneof), oneof::structs::Parent::SingleFieldOneof>);

    EXPECT_FALSE(message.single_field_oneof.has_single());

    message.single_field_oneof.set_single("text");
    EXPECT_TRUE(message.single_field_oneof.has_single());
    EXPECT_EQ(message.single_field_oneof.single(), "text");
}

TEST(OneofBasic, SyntheticOneofIsIgnored) {
    [[maybe_unused]] oneof::structs::Parent message;
    static_assert(std::is_same_v<decltype(message.field_before), std::optional<std::string>>);
}

USERVER_NAMESPACE_END
