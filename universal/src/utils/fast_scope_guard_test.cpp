#include <userver/utils/fast_scope_guard.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(FastScopeGuard, Dtr) {
  /// [FastScopeGuard]
  int x = 0;
  {
    utils::FastScopeGuard d([&]() noexcept { x = 1; });
    EXPECT_EQ(x, 0);
  }
  EXPECT_EQ(x, 1);
  /// [FastScopeGuard]
}

TEST(FastScopeGuard, Cancel) {
  int x = 0;
  {
    utils::FastScopeGuard d([&]() noexcept { x = 1; });
    EXPECT_EQ(x, 0);
    d.Release();
  }
  EXPECT_EQ(x, 0);
}

TEST(FastScopeGuard, Exception) {
  int x = 0;
  try {
    utils::FastScopeGuard d([&]() noexcept { x = 1; });
    EXPECT_EQ(x, 0);
    throw std::runtime_error("");
  } catch (const std::runtime_error&) {
  }
  EXPECT_EQ(x, 1);
}

TEST(FastScopeGuard, Move) {
  int x = 0;
  {
    auto guard = [&] {
      utils::FastScopeGuard d([&]() noexcept { x = 1; });
      return d;
    }();
    EXPECT_EQ(x, 0);
  }
  EXPECT_EQ(x, 1);
}

USERVER_NAMESPACE_END
