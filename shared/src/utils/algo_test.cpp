#include <utils/algo.hpp>

#include <map>
#include <string>
#include <unordered_map>

#include <gtest/gtest.h>

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
