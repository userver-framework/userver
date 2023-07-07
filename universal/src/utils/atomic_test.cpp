#include <userver/utils/atomic.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(AtomicTest, AtomicUpdate) {
  std::atomic<int> atomic(10);

  utils::AtomicUpdate(atomic, [](int x) { return x * 2 + 2; });
  EXPECT_EQ(atomic.load(), 22);
}

TEST(AtomicTest, AtomicMin) {
  std::atomic<int> atomic(10);

  utils::AtomicMin(atomic, 5);
  EXPECT_EQ(atomic.load(), 5);

  utils::AtomicMin(atomic, 20);
  EXPECT_EQ(atomic.load(), 5);
}

TEST(AtomicTest, AtomicMax) {
  std::atomic<int> atomic(10);

  utils::AtomicMax(atomic, 5);
  EXPECT_EQ(atomic.load(), 10);

  utils::AtomicMax(atomic, 20);
  EXPECT_EQ(atomic.load(), 20);
}

USERVER_NAMESPACE_END
