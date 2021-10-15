#include <userver/utils/thread_name.hpp>

#include <gtest/gtest.h>

TEST(ThreadName, SetSelf) {
  auto old_name = utils::GetCurrentThreadName();
  auto new_name = "12345";

  utils::SetCurrentThreadName(new_name);
  EXPECT_EQ(new_name, utils::GetCurrentThreadName());

  utils::SetCurrentThreadName(old_name);
  EXPECT_EQ(old_name, utils::GetCurrentThreadName());
}

TEST(ThreadName, Invalid) {
  EXPECT_ANY_THROW(utils::SetCurrentThreadName({"a\0b", 3}));
}
