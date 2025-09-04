#include <gtest/gtest.h>

#include <optional>
#include <unordered_map>
#include <vector>

#include <userver/proto-structs/oneof.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::tests {

enum TestEnum { kValue0 = 0, kValue1 = 1, kValue2 = 2 };

struct TestStruct {
    std::string f1;
};

using TestOneof = proto_structs::Oneof<int32_t, int32_t, std::string, TestEnum, TestStruct>;

void CheckAlternativeSet(const TestOneof& oneof, std::size_t set_index) {
    ASSERT_EQ(oneof.GetIndex(), set_index);

    bool found = false;

    for (std::size_t i = 0; i < TestOneof::kSize; ++i) {
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
    static_assert(traits::Oneof<TestOneof>);
    static_assert(traits::Oneof<const volatile TestOneof>);
    static_assert(TestOneof::kSize == 5);

    static_assert(!traits::Oneof<TestOneof&>);
    static_assert(!traits::Oneof<int32_t>);
    static_assert(!traits::Oneof<std::pair<int32_t, int32_t>>);

    static_assert(std::is_same_v<int32_t, OneofAlternativeType<0, TestOneof>>);
    static_assert(std::is_same_v<int32_t, OneofAlternativeType<1, TestOneof>>);
    static_assert(std::is_same_v<std::string, OneofAlternativeType<2, TestOneof>>);

    static_assert(traits::OneofField<int32_t>);
    static_assert(traits::OneofField<const volatile int32_t>);
    static_assert(traits::OneofField<uint32_t>);
    static_assert(traits::OneofField<int64_t>);
    static_assert(traits::OneofField<uint64_t>);
    static_assert(traits::OneofField<bool>);
    static_assert(traits::OneofField<float>);
    static_assert(traits::OneofField<double>);
    static_assert(traits::OneofField<std::string>);
    static_assert(traits::OneofField<const volatile TestEnum>);
    static_assert(traits::OneofField<TestStruct>);

    static_assert(!traits::OneofField<std::vector<int32_t>>);
    static_assert(!traits::OneofField<std::optional<int32_t>>);
    static_assert(!traits::OneofField<std::map<std::string, std::string>>);
    static_assert(!traits::OneofField<std::unordered_map<int32_t, int32_t>>);
}

TEST(OneofTest, Ctor) {
    TestOneof default_oneof;

    CheckAlternativeSet(default_oneof, kOneofNpos);

    TestOneof oneof(std::in_place_index<2>, "hello world");

    CheckAlternativeSet(oneof, 2);
    EXPECT_EQ(oneof.Get<2>(), "hello world");

    TestOneof oneof_copy(oneof);

    CheckAlternativeSet(oneof_copy, 2);
    EXPECT_EQ(oneof_copy.Get<2>(), "hello world");

    oneof.Get<2>() = "test1";
    oneof_copy = oneof;

    CheckAlternativeSet(oneof_copy, 2);
    EXPECT_EQ(oneof_copy.Get<2>(), "test1");

    oneof.Get<2>() = "test2";
    TestOneof oneof_move(std::move(oneof));

    CheckAlternativeSet(oneof_move, 2);
    EXPECT_EQ(oneof_move.Get<2>(), "test2");

    oneof_copy = std::move(oneof_move);

    CheckAlternativeSet(oneof_copy, 2);
    EXPECT_EQ(oneof_copy.Get<2>(), "test2");
}

TEST(OneofTest, GetSetEmplace) {
    TestOneof oneof;

    oneof.Set<0>(0);

    CheckAlternativeSet(oneof, 0);
    EXPECT_EQ(oneof.Get<0>(), 0);

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

    oneof.Set<4>({.f1 = "test"});

    CheckAlternativeSet(oneof, 4);
    EXPECT_EQ(std::move(oneof).Get<4>().f1, "test");
}

TEST(OneofTest, Clear) {
    TestOneof oneof;

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
