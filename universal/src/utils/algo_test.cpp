#include <userver/utils/algo.hpp>

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(UtilsAlgo, StrCat) {
  EXPECT_EQ(utils::StrCat("a", std::string_view{"bbb"}, std::string{"cc"},
                          static_cast<const char*>("dddd")),
            "abbbccdddd");
}

TEST(UtilsAlgo, FindOrNullptrMaps) {
  std::map<std::string, int> m = {{"1", 2}};
  std::unordered_map<std::string, int> um = {{"1", 2}};

  EXPECT_FALSE(utils::FindOrNullptr(m, "2"));
  EXPECT_FALSE(utils::FindOrNullptr(um, "2"));

  ASSERT_TRUE(utils::FindOrNullptr(m, "1"));
  ASSERT_TRUE(utils::FindOrNullptr(um, "1"));

  EXPECT_EQ(*utils::FindOrNullptr(m, "1"), 2);
  EXPECT_EQ(*utils::FindOrNullptr(um, "1"), 2);
}

TEST(UtilsAlgo, FindOrDefaultMaps) {
  constexpr int kFallback = 42;
  std::map<std::string, int> m = {{"1", 2}};
  std::unordered_map<std::string, int> um = {{"1", 2}};

  EXPECT_EQ(utils::FindOrDefault(m, "2", kFallback), kFallback);
  EXPECT_EQ(utils::FindOrDefault(um, "2", kFallback), kFallback);

  ASSERT_EQ(utils::FindOrDefault(m, "1", kFallback), 2);
  ASSERT_EQ(utils::FindOrDefault(um, "1", kFallback), 2);

  EXPECT_EQ(utils::FindOrDefault(m, "1", kFallback), 2);
  EXPECT_EQ(utils::FindOrDefault(um, "1", kFallback), 2);
}

TEST(UtilsAlgo, FindOptional) {
  std::map<std::string, int> m = {{"1", 2}};
  std::unordered_map<std::string, int> um = {{"1", 2}};

  EXPECT_FALSE(utils::FindOptional(m, "2"));
  EXPECT_FALSE(utils::FindOptional(um, "2"));

  EXPECT_EQ(utils::FindOptional(m, "1"), 2);
  EXPECT_EQ(utils::FindOptional(um, "1"), 2);
}

TEST(UtilsAlgo, Erase) {
  std::map<std::string, int> m = {{"1", 2}, {"2", 1}};
  std::unordered_map<std::string, int> um = {{"1", 2}, {"2", 1}};
  std::unordered_multiset<int> ums = {1, 1, 1, 2, 3, 4};
  std::vector<int> v = {1, 1, 1, 2, 3, 4};
  const auto one_count = std::count(cbegin(v), cend(v), 1);

  ASSERT_EQ(utils::Erase(m, "1"), 1);
  EXPECT_EQ(m, (std::map<std::string, int>{{"2", 1}}));

  ASSERT_EQ(utils::Erase(um, "1"), 1);
  EXPECT_EQ(um, (std::unordered_map<std::string, int>{{"2", 1}}));

  ASSERT_EQ(utils::Erase(ums, 1), one_count);
  EXPECT_EQ(ums, (std::unordered_multiset<int>{2, 3, 4}));

  ASSERT_EQ(utils::Erase(v, 1), one_count);
  EXPECT_EQ(v, (std::vector<int>{2, 3, 4}));
}

TEST(UtilsAlgo, EraseIf) {
  std::map<std::string, int> m = {{"1", 2}, {"2", 1}};
  std::unordered_map<std::string, int> um = {{"1", 2}, {"2", 1}};
  std::unordered_multiset<int> ums = {1, 1, 1, 2, 3, 4};
  std::vector<int> v = {1, 1, 1, 2, 3, 4};

  auto value_greater_than_one = [](const auto& p) { return p.second > 1; };
  auto is_odd = [](int v) { return v % 2 == 1; };
  const auto odd_count = std::count_if(cbegin(v), cend(v), is_odd);

  EXPECT_EQ(utils::EraseIf(m, value_greater_than_one), 1);
  EXPECT_EQ(m, (std::map<std::string, int>{{"2", 1}}));

  EXPECT_EQ(utils::EraseIf(um, value_greater_than_one), 1);
  EXPECT_EQ(um, (std::unordered_map<std::string, int>{{"2", 1}}));

  EXPECT_EQ(utils::EraseIf(ums, is_odd), odd_count);
  EXPECT_EQ(ums, (std::unordered_multiset<int>{2, 4}));

  EXPECT_EQ(utils::EraseIf(v, is_odd), odd_count);
  EXPECT_EQ(v, (std::vector<int>{2, 4}));
}

TEST(UtilsAlgo, AsContainer) {
  const std::vector<int> v = {1, 1, 2, 2, 3, 3};
  const auto s = utils::AsContainer<std::set<int>>(v);
  EXPECT_EQ(s, (std::set<int>{1, 2, 3}));
  EXPECT_EQ(utils::AsContainer<std::vector<int>>(s),
            (std::vector<int>{1, 2, 3}));

  std::vector<std::string> vs = {"123", "456", "789"};
  const auto ss = utils::AsContainer<std::set<std::string>>(std::move(vs));
  EXPECT_EQ(ss, (std::set<std::string>{"123", "456", "789"}));
}

TEST(UtilsAlgo, ContainsIf) {
  const std::unordered_map<std::string, int> um = {{"1", 2}, {"2", 1}};
  const std::unordered_multiset<int> ums = {1, 1, 1, 2, 3, 4};
  const std::vector<int> v = {2, 4, 6, 8};

  auto value_greater_than_one = [](const auto& p) { return p.second > 1; };
  auto is_odd = [](int v) { return v % 2 == 1; };

  EXPECT_TRUE(utils::ContainsIf(um, value_greater_than_one));
  EXPECT_TRUE(utils::ContainsIf(ums, is_odd));
  EXPECT_FALSE(utils::ContainsIf(v, is_odd));
}

USERVER_NAMESPACE_END
