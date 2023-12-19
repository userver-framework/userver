#include <userver/utils/thread_name.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(ThreadName, SetSelf) {
  const auto old_name = utils::GetCurrentThreadName();
  constexpr std::string_view new_name = "12345";

  utils::SetCurrentThreadName(new_name);
  EXPECT_EQ(new_name, utils::GetCurrentThreadName());

  utils::SetCurrentThreadName(old_name);
  EXPECT_EQ(old_name, utils::GetCurrentThreadName());
}

TEST(ThreadName, Invalid) {
  EXPECT_ANY_THROW(utils::SetCurrentThreadName({"a\0b", 3}));
}

TEST(ThreadName, Scoped) {
  const auto old_name = utils::GetCurrentThreadName();
  constexpr std::string_view new_name = "in_scope";

  {
    utils::CurrentThreadNameGuard thread_name_guard(new_name);
    EXPECT_EQ(new_name, utils::GetCurrentThreadName());
  }
  EXPECT_EQ(old_name, utils::GetCurrentThreadName());
}

USERVER_NAMESPACE_END
