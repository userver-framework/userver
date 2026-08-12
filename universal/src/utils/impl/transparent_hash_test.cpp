#include <userver/utils/impl/transparent_hash.hpp>

#include <gtest/gtest.h>

#include <userver/utils/algo.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/str_icase.hpp>

#include "utils/overloaded_address_operator_test.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

struct StringViewable final {
    std::string_view value;

    /*implicit*/ operator std::string_view() const { return value; }
};

template <typename Other>
bool operator==(StringViewable lhs, Other rhs) {
    return lhs.value == rhs;
}

bool operator==(std::string_view lhs, StringViewable rhs) { return lhs == rhs.value; }

}  // namespace

TEST(TransparentSet, Find) {
    utils::impl::TransparentSet<std::string> set;
    set.insert("foo");

    constexpr std::string_view expected = "foo";
    EXPECT_EQ(*set.find("foo"), expected);
    EXPECT_EQ(*set.find(std::string{"foo"}), expected);
    EXPECT_EQ(*set.find(std::string_view{"foo"}), expected);
    EXPECT_EQ(*set.find(StringViewable{"foo"}), expected);

    EXPECT_EQ(set.find("bar"), set.end());
    EXPECT_EQ(set.find("Foo"), set.end());

    const auto const_set = std::move(set);
    EXPECT_EQ(*const_set.find("foo"), expected);
    EXPECT_EQ(const_set.find("bar"), const_set.end());
}

TEST(TransparentMap, Find) {
    using Map = utils::impl::TransparentMap<std::string, int>;
    using Entry = std::pair<const std::string, int>;
    static_assert(meta::IsUniqueMap<Map>);

    Map map;
    map.emplace("foo", 4);

    const Entry expected{"foo", 4};
    EXPECT_EQ(*map.find("foo"), expected);
    EXPECT_EQ(*map.find(std::string{"foo"}), expected);
    EXPECT_EQ(*map.find(std::string_view{"foo"}), expected);
    EXPECT_EQ(*map.find(StringViewable{"foo"}), expected);

    EXPECT_EQ(map.find("bar"), map.end());
    EXPECT_EQ(map.find("Foo"), map.end());

    const auto const_map = std::move(map);
    EXPECT_EQ(*const_map.find("foo"), expected);
    EXPECT_EQ(const_map.find("bar"), const_map.end());
}

TEST(TransparentMap, FindOrNullptr) {
    utils::impl::TransparentMap<std::string, int> map;
    map.emplace("foo", 4);

    constexpr int expected = 4;
    EXPECT_EQ(*utils::FindOrNullptr(map, "foo"), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, std::string{"foo"}), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, std::string_view{"foo"}), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, StringViewable{"foo"}), expected);

    EXPECT_EQ(utils::FindOrNullptr(map, "bar"), nullptr);
    EXPECT_EQ(utils::FindOrNullptr(map, "Foo"), nullptr);

    const auto const_map = std::move(map);
    EXPECT_EQ(*utils::FindOrNullptr(const_map, "foo"), expected);
    EXPECT_EQ(utils::FindOrNullptr(const_map, "bar"), nullptr);
}

TEST(TransparentMap, FindTransparentOrNullptrAddressof) {
    utils::impl::TransparentMap<std::string, utils::OverloadedAddressOperator> m{{{"1"}, {}}};
    const auto* ptr = utils::FindOrNullptr(m, "1");
    ASSERT_TRUE(ptr);
}

TEST(TransparentMap, CustomValue) {
    using Map = utils::impl::TransparentMap<StringViewable, int>;
    static_assert(meta::IsUniqueMap<Map>);

    Map map;
    map.emplace(StringViewable{"foo"}, 4);

    constexpr int expected = 4;
    EXPECT_EQ(*utils::FindOrNullptr(map, "foo"), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, std::string{"foo"}), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, std::string_view{"foo"}), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, StringViewable{"foo"}), expected);
}

TEST(TransparentMap, CustomTransparentHash) {
    using Map = utils::impl::TransparentMap<std::string, int, utils::StrIcaseHash, utils::StrIcaseEqual>;
    static_assert(meta::IsUniqueMap<Map>);

    Map map;
    map.emplace("foo", 4);

    constexpr int expected = 4;
    EXPECT_EQ(*utils::FindOrNullptr(map, "foo"), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, StringViewable{"foo"}), expected);

    EXPECT_EQ(*utils::FindOrNullptr(map, "Foo"), expected);
    EXPECT_EQ(*utils::FindOrNullptr(map, "FOO"), expected);
    EXPECT_EQ(utils::FindOrNullptr(map, "bar"), nullptr);
}

USERVER_NAMESPACE_END
