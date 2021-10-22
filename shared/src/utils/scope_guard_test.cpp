#include <gtest/gtest.h>

#ifndef NDEBUG
#define NDEBUG  // for ExceptionSuppression test
#endif

#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

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

TEST(ScopeGuard, ExceptionPropagation) {
  struct TestException : std::exception {};

  EXPECT_THROW([] { utils::ScopeGuard guard{[] { throw TestException{}; }}; }(),
               TestException);
}

TEST(ScopeGuard, ExceptionSuppression) {
  struct TestExceptionInner : std::exception {};
  struct TestExceptionOuter : std::exception {};

  EXPECT_THROW(
      [] {
        utils::ScopeGuard guard{[] { throw TestExceptionInner{}; }};
        throw TestExceptionOuter{};
      }(),
      TestExceptionOuter);
}

USERVER_NAMESPACE_END
