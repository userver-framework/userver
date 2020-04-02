#include <gtest/gtest.h>

#include <array>

#include <utils/meta.hpp>

TEST(Meta, IsArray) {
  // quick in-place tests
  EXPECT_TRUE((meta::is_array<std::array<int, 5>>::value));
  EXPECT_FALSE(meta::is_array<int>::value);
}
