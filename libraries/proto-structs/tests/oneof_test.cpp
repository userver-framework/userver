#include <gtest/gtest.h>

#include <chrono>
#include <optional>
#include <unordered_map>
#include <vector>

#include <userver/proto-structs/any.hpp>
#include <userver/proto-structs/io/std/chrono/duration.hpp>
#include <userver/proto-structs/io/userver/utils/box.hpp>
#include <userver/proto-structs/io/userver/utils/strong_typedef.hpp>
#include <userver/proto-structs/oneof.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/box.hpp>
#include <userver/utils/strong_typedef.hpp>

#include "messages.pb.h"
#include "struct_simple.hpp"

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

enum TestEnum {
    kValue0 = 0,
    kValue1 = 1,
    kValue2 = 2
};

using RawOneof = proto_structs::Oneof<int32_t, int32_t, std::string, TestEnum, structs::Simple>;

struct CustomOneof : RawOneof {};

void CheckAlternativeSet(const RawOneof& oneof, std::size_t set_index) {
    ASSERT_EQ(oneof.GetIndex(), set_index);

    bool found = false;

    for (std::size_t i = 0; i < RawOneof::kSize; ++i) {
        if (i != set_index) {
            ASSERT_FALSE(oneof.Contains(i));
        } else {
            ASSERT_TRUE(oneof.Contains(i));
            found = true;
        }
    }

    ASSERT_EQ(oneof.ContainsAny(), found);

    if (found) {
        ASSERT_TRUE(oneof);
    } else {
        ASSERT_FALSE(oneof);
    }
}

TEST(OneofTest, Traits) {
    struct Tag {};

    static_assert(traits::Oneof<RawOneof>);
    static_assert(traits::Oneof<const volatile RawOneof>);
    static_assert(traits::Oneof<CustomOneof>);
    static_assert(traits::Oneof<const volatile CustomOneof>);

    static_assert(RawOneof::kSize == 5);
    static_assert(CustomOneof::kSize == 5);

    static_assert(!traits::Oneof<RawOneof&>);
    static_assert(!traits::Oneof<CustomOneof&>);
    static_assert(!traits::Oneof<int32_t>);
    static_assert(!traits::Oneof<std::pair<int32_t, int32_t>>);

    static_assert(std::is_same_v<int32_t, OneofAlternativeType<0, RawOneof>>);
    static_assert(std::is_same_v<int32_t, OneofAlternativeType<1, RawOneof>>);
    static_assert(std::is_same_v<std::string, OneofAlternativeType<2, RawOneof>>);
    static_assert(std::is_same_v<TestEnum, OneofAlternativeType<3, RawOneof>>);
    static_assert(std::is_same_v<structs::Simple, OneofAlternativeType<4, RawOneof>>);

    static_assert(std::is_same_v<int32_t, OneofAlternativeType<0, CustomOneof>>);
    static_assert(std::is_same_v<int32_t, OneofAlternativeType<1, CustomOneof>>);
    static_assert(std::is_same_v<std::string, OneofAlternativeType<2, CustomOneof>>);
    static_assert(std::is_same_v<TestEnum, OneofAlternativeType<3, CustomOneof>>);
    static_assert(std::is_same_v<structs::Simple, OneofAlternativeType<4, CustomOneof>>);
}

TEST(OneofTest, Ctor) {
    RawOneof default_oneof;

    CheckAlternativeSet(default_oneof, kOneofNpos);

    RawOneof oneof(std::in_place_index<2>, "hello world");

    CheckAlternativeSet(oneof, 2);
    EXPECT_EQ(oneof.Get<2>(), "hello world");

    RawOneof oneof_copy(oneof);

    CheckAlternativeSet(oneof_copy, 2);
    EXPECT_EQ(oneof_copy.Get<2>(), "hello world");

    oneof.Get<2>() = "test1";
    oneof_copy = oneof;

    CheckAlternativeSet(oneof_copy, 2);
    EXPECT_EQ(oneof_copy.Get<2>(), "test1");

    oneof.Get<2>() = "test2";
    RawOneof oneof_move(std::move(oneof));

    CheckAlternativeSet(oneof_move, 2);
    EXPECT_EQ(oneof_move.Get<2>(), "test2");

    oneof_copy = std::move(oneof_move);

    CheckAlternativeSet(oneof_copy, 2);
    EXPECT_EQ(oneof_copy.Get<2>(), "test2");
}

TEST(OneofTest, GetSetEmplace) {
    RawOneof oneof;

    oneof.Set<0>(0);

    CheckAlternativeSet(oneof, 0);
    EXPECT_EQ(oneof.Get<0>(), 0);
    UEXPECT_THROW_MSG(static_cast<void>(oneof.Get<1>()), OneofAccessError, "index = 1");

    oneof.Set<0>(42);

    CheckAlternativeSet(oneof, 0);
    EXPECT_EQ(oneof.Get<0>(), 42);

    EXPECT_EQ(oneof.Emplace<1>(1001), 1001);
    CheckAlternativeSet(oneof, 1);
    EXPECT_EQ(oneof.Get<1>(), 1001);

    ++oneof.Get<1>();

    CheckAlternativeSet(oneof, 1);
    EXPECT_EQ(oneof.Get<1>(), 1002);

    oneof.Set<2>("hello world");

    CheckAlternativeSet(oneof, 2);
    EXPECT_EQ(oneof.Get<2>(), "hello world");

    std::string str = "some string";

    EXPECT_EQ(oneof.Emplace<2>(str.begin() + 5, str.end()), "string");
    CheckAlternativeSet(oneof, 2);

    str += "!";
    oneof.Set<2>(std::move(str));

    CheckAlternativeSet(oneof, 2);
    EXPECT_EQ(oneof.Get<2>(), "some string!");

    oneof.Set<3>(kValue1);

    CheckAlternativeSet(oneof, 3);
    EXPECT_EQ(oneof.Get<3>(), kValue1);

    oneof.Set<4>({.f1 = 11});

    CheckAlternativeSet(oneof, 4);
    EXPECT_EQ(std::move(oneof).Get<4>().f1, 11);

    oneof.GetMutable<0>() = 6;

    CheckAlternativeSet(oneof, 0);
    EXPECT_EQ(oneof.Get<0>(), 6);
}

TEST(OneofTest, Clear) {
    RawOneof oneof;

    EXPECT_NO_THROW(oneof.ClearOneof());
    CheckAlternativeSet(oneof, kOneofNpos);

    oneof.Set<0>(1);

    EXPECT_NO_THROW(oneof.Clear(1));
    CheckAlternativeSet(oneof, 0);
    EXPECT_EQ(oneof.Get<0>(), 1);

    EXPECT_NO_THROW(oneof.Clear(0));
    CheckAlternativeSet(oneof, kOneofNpos);

    oneof.Set<1>(2);

    EXPECT_NO_THROW(oneof.ClearOneof());
    CheckAlternativeSet(oneof, kOneofNpos);
}

}  // namespace proto_structs::tests

USERVER_NAMESPACE_END
