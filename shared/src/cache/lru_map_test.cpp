#include <gtest/gtest.h>

#include <cache/lru_map.hpp>

using Lru = cache::LruMap<int, int>;

TEST(Lru, SetGet) {
  Lru cache(10);
  EXPECT_EQ(nullptr, cache.Get(1));
  cache.Put(1, 2);
  EXPECT_EQ(2, cache.GetOr(1, -1));
  cache.Put(1, 3);
  EXPECT_EQ(3, cache.GetOr(1, -1));
}

TEST(Lru, Erase) {
  Lru cache(10);
  cache.Put(1, 2);
  cache.Erase(1);
  EXPECT_EQ(nullptr, cache.Get(1));
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

TEST(Lru, OverflowSetMaxSizeShrink) {
  Lru cache(3);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(1, 11);
  cache.Put(3, 30);
  cache.SetMaxSize(2);

  EXPECT_EQ(11, cache.GetOr(1, -1));
  EXPECT_EQ(-1, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
}

TEST(Lru, OverflowSetMaxSizeExpand) {
  Lru cache(3);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(1, 11);
  cache.Put(3, 30);
  cache.SetMaxSize(5);
  cache.Put(4, 40);
  cache.Put(5, 50);

  EXPECT_EQ(11, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
  EXPECT_EQ(40, cache.GetOr(4, -1));
  EXPECT_EQ(50, cache.GetOr(5, -1));
  EXPECT_EQ(-1, cache.GetOr(6, -1));

  cache.Put(6, 60);
  EXPECT_EQ(-1, cache.GetOr(1, -1));
  EXPECT_EQ(20, cache.GetOr(2, -1));
  EXPECT_EQ(30, cache.GetOr(3, -1));
  EXPECT_EQ(40, cache.GetOr(4, -1));
  EXPECT_EQ(50, cache.GetOr(5, -1));
  EXPECT_EQ(60, cache.GetOr(6, -1));
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

TEST(Lru, VisitAll) {
  Lru cache(10);
  cache.Put(1, 10);
  cache.Put(2, 20);
  cache.Put(3, 30);

  std::size_t sum_keys = 0;
  std::size_t sum_values = 0;
  // mutable - to test VisitAll compilation with non-const operator()
  cache.VisitAll(
      [&sum_keys, &sum_values](const auto& key, const auto& value) mutable {
        sum_keys += key;
        sum_values += value;
      });

  EXPECT_EQ(6, sum_keys);
  EXPECT_EQ(60, sum_values);
}
