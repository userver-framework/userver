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
