#include <utest/utest.hpp>

#include <cache/lru_cache.hpp>

using Lru = cache::LRU<int, int>;

TEST(Lru, SetGet) {
  Lru cache(10);
  EXPECT_EQ(nullptr, cache.Get(1));
  cache.Put(1, 2);
  EXPECT_EQ(2, cache.GetOr(1, -1));
  cache.Put(1, 3);
  EXPECT_EQ(3, cache.GetOr(1, -1));
}

TEST(Lru, MultipleSet) {
  Lru cache(10);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(3, 30);
  EXPECT_EQ(10, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, Overflow) {
  Lru cache(2);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(3, 30);
  EXPECT_EQ(-1, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, OverflowUpdate) {
  Lru cache(2);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(1, 11);
  cache.Put(3, 30);

  EXPECT_EQ(11, cache.GetOr(1, -1));
  EXPECT_EQ(-1, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, OverflowGet) {
  Lru cache(2);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Get(1);
  cache.Put(3, 30);

  EXPECT_EQ(10, cache.GetOr(1, -1));
  EXPECT_EQ(-1, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}
