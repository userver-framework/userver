#include <userver/utils/impl/transparent_hash.hpp>

#include <gtest/gtest.h>

#include <userver/utils/meta.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using utils::impl::FindTransparent;
using utils::impl::FindTransparentOrNullptr;
using utils::impl::TransparentInsertOrAssign;

struct StringViewable final {
  std::string_view value;

  /*implicit*/ operator std::string_view() const { return value; }
};

template <typename Other>
bool operator==(StringViewable lhs, Other rhs) {
  return lhs.value == rhs;
}

bool operator==(std::string_view lhs, StringViewable rhs) {
  return lhs == rhs.value;
}

}  // namespace

TEST(TransparentSet, FindTransparent) {
  utils::impl::TransparentSet<std::string> set;
  set.insert("foo");

  constexpr std::string_view expected = "foo";
  EXPECT_EQ(*FindTransparent(set, "foo"), expected);
  EXPECT_EQ(*FindTransparent(set, std::string{"foo"}), expected);
  EXPECT_EQ(*FindTransparent(set, std::string_view{"foo"}), expected);
  EXPECT_EQ(*FindTransparent(set, StringViewable{"foo"}), expected);

  EXPECT_EQ(FindTransparent(set, "bar"), set.end());
  EXPECT_EQ(FindTransparent(set, "Foo"), set.end());

  const auto const_set = std::move(set);
  EXPECT_EQ(*FindTransparent(const_set, "foo"), expected);
  EXPECT_EQ(FindTransparent(const_set, "bar"), const_set.end());
}

TEST(TransparentMap, FindTransparent) {
  using Map = utils::impl::TransparentMap<std::string, int>;
  using Entry = std::pair<const std::string, int>;
  static_assert(meta::kIsUniqueMap<Map>);

  Map map;
  map.emplace("foo", 4);

  const Entry expected{"foo", 4};
  EXPECT_EQ(*FindTransparent(map, "foo"), expected);
  EXPECT_EQ(*FindTransparent(map, std::string{"foo"}), expected);
  EXPECT_EQ(*FindTransparent(map, std::string_view{"foo"}), expected);
  EXPECT_EQ(*FindTransparent(map, StringViewable{"foo"}), expected);

  EXPECT_EQ(FindTransparent(map, "bar"), map.end());
  EXPECT_EQ(FindTransparent(map, "Foo"), map.end());

  const auto const_map = std::move(map);
  EXPECT_EQ(*FindTransparent(const_map, "foo"), expected);
  EXPECT_EQ(FindTransparent(const_map, "bar"), const_map.end());
}

TEST(TransparentMap, FindTransparentOrNullptr) {
  utils::impl::TransparentMap<std::string, int> map;
  map.emplace("foo", 4);

  constexpr int expected = 4;
  EXPECT_EQ(*FindTransparentOrNullptr(map, "foo"), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, std::string{"foo"}), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, std::string_view{"foo"}), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, StringViewable{"foo"}), expected);

  EXPECT_EQ(FindTransparentOrNullptr(map, "bar"), nullptr);
  EXPECT_EQ(FindTransparentOrNullptr(map, "Foo"), nullptr);

  const auto const_map = std::move(map);
  EXPECT_EQ(*FindTransparentOrNullptr(const_map, "foo"), expected);
  EXPECT_EQ(FindTransparentOrNullptr(const_map, "bar"), nullptr);
}

TEST(TransparentMap, CustomValue) {
  using Map = utils::impl::TransparentMap<StringViewable, int>;
  static_assert(meta::kIsUniqueMap<Map>);

  Map map;
  map.emplace(StringViewable{"foo"}, 4);

  constexpr int expected = 4;
  EXPECT_EQ(*FindTransparentOrNullptr(map, "foo"), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, std::string{"foo"}), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, std::string_view{"foo"}), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, StringViewable{"foo"}), expected);
}

TEST(TransparentMap, CustomTransparentHash) {
  using Map = utils::impl::TransparentMap<std::string, int, utils::StrIcaseHash,
                                          utils::StrIcaseEqual>;
  static_assert(meta::kIsUniqueMap<Map>);

  Map map;
  map.emplace("foo", 4);

  constexpr int expected = 4;
  EXPECT_EQ(*FindTransparentOrNullptr(map, "foo"), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, StringViewable{"foo"}), expected);

  EXPECT_EQ(*FindTransparentOrNullptr(map, "Foo"), expected);
  EXPECT_EQ(*FindTransparentOrNullptr(map, "FOO"), expected);
  EXPECT_EQ(FindTransparentOrNullptr(map, "bar"), nullptr);
}

TEST(TransparentMap, TransparentInsertOrAssign) {
  utils::impl::TransparentMap<std::string, std::unique_ptr<int>> map;
  TransparentInsertOrAssign(map, std::string_view{"foo"},
                            std::make_unique<int>(4));
  EXPECT_EQ(**FindTransparentOrNullptr(map, "foo"), 4);

  TransparentInsertOrAssign(map, std::string_view{"foo"},
                            std::make_unique<int>(5));
  EXPECT_EQ(**FindTransparentOrNullptr(map, "foo"), 5);

  TransparentInsertOrAssign(map, std::string{"bar"}, std::make_unique<int>(4));
  EXPECT_EQ(**FindTransparentOrNullptr(map, "bar"), 4);

  TransparentInsertOrAssign(map, std::string{"bar"}, std::make_unique<int>(5));
  EXPECT_EQ(**FindTransparentOrNullptr(map, "bar"), 5);

  constexpr std::string_view kLongString = "a string that does not fit in SSO";
  std::string not_moved_key{kLongString};
  TransparentInsertOrAssign(map, not_moved_key, std::make_unique<int>(4));
  EXPECT_EQ(**FindTransparentOrNullptr(map, not_moved_key), 4);
  EXPECT_EQ(not_moved_key, kLongString);
}

USERVER_NAMESPACE_END
