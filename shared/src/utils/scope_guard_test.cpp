#include <userver/utils/scope_guard.hpp>

#include <gtest/gtest.h>

#include <userver/utest/death_tests.hpp>

USERVER_NAMESPACE_BEGIN

TEST(ScopeGuard, Dtr) {
  /// [ScopeGuard usage example]
  int x = 0;
  {
    utils::ScopeGuard d([&x] { x = 1; });
    EXPECT_EQ(x, 0);
  }
  EXPECT_EQ(x, 1);
  /// [ScopeGuard usage example]
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
  testing::FLAGS_gtest_death_test_style = "threadsafe";

  struct TestExceptionInner : std::exception {};
  struct TestExceptionOuter : std::exception {};

  const auto test_body = [] {
    utils::ScopeGuard guard{[] { throw TestExceptionInner{}; }};
    throw TestExceptionOuter{};
  };

#ifdef NDEBUG
  EXPECT_THROW(test_body(), TestExceptionOuter);
#else
  UEXPECT_DEATH(test_body(), "exception is thrown during stack unwinding");
#endif
}

USERVER_NAMESPACE_END
