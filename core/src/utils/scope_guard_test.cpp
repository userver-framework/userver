#include <utest/utest.hpp>
#include <utils/scope_guard.hpp>

TEST(ScopeGuard, Dtr) {
  int x = 0;
  {
    utils::ScopeGuard d([&x] { x = 1; });
    EXPECT_EQ(x, 0);
  }
  EXPECT_EQ(x, 1);
}

TEST(ScopeGuard, Cancel) {
  int x = 0;
  {
    utils::ScopeGuard d([&x] { x = 1; });
    EXPECT_EQ(x, 0);
    d.Release();
  }
  EXPECT_EQ(x, 0);
}

TEST(ScopeGuard, Exception) {
  int x = 0;
  try {
    utils::ScopeGuard d([&x] { x = 1; });
    EXPECT_EQ(x, 0);
    throw std::runtime_error("");
  } catch (const std::runtime_error&) {
  }
  EXPECT_EQ(x, 1);
}
