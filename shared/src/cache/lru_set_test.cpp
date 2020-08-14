#include <gtest/gtest.h>

#include <cache/lru_set.hpp>

using Lru = cache::LruSet<int>;

TEST(LruSet, SetGet) {
  Lru cache(10);
  EXPECT_EQ(false, cache.Has(1));
  cache.Put(1);
  EXPECT_EQ(true, cache.Has(1));
  cache.Put(1);
  EXPECT_EQ(true, cache.Has(1));
}

TEST(LruSet, Erase) {
  Lru cache(10);
  cache.Put(1);
  cache.Erase(1);
  EXPECT_EQ(false, cache.Has(1));
}

TEST(LruSet, MultipleSet) {
  Lru cache(10);
  cache.Put(1);
  cache.Put(2);
  cache.Put(3);
  EXPECT_EQ(true, cache.Has(1));
  EXPECT_EQ(true, cache.Has(2));
  EXPECT_EQ(true, cache.Has(3));
}

TEST(LruSet, Overflow) {
  Lru cache(2);
  cache.Put(1);
  cache.Put(2);
  cache.Put(3);
  EXPECT_EQ(false, cache.Has(1));
  EXPECT_EQ(true, cache.Has(2));
  EXPECT_EQ(true, cache.Has(3));
}

TEST(LruSet, OverflowUpdate) {
  Lru cache(2);
  cache.Put(1);
  cache.Put(2);
  cache.Put(1);
  cache.Put(3);

  EXPECT_EQ(true, cache.Has(1));
  EXPECT_EQ(false, cache.Has(2));
  EXPECT_EQ(true, cache.Has(3));
}

TEST(LruSet, OverflowSetMaxSizeShrink) {
  Lru cache(3);
  cache.Put(1);
  cache.Put(2);
  cache.Put(1);
  cache.Put(3);
  cache.SetMaxSize(2);

  EXPECT_EQ(true, cache.Has(1));
  EXPECT_EQ(false, cache.Has(2));
  EXPECT_EQ(true, cache.Has(3));
}

TEST(LruSet, OverflowSetMaxSizeExpand) {
  Lru cache(3);
  cache.Put(1);
  cache.Put(2);
  cache.Put(1);
  cache.Put(3);
  cache.SetMaxSize(5);
  cache.Put(4);
  cache.Put(5);

  EXPECT_EQ(true, cache.Has(1));
  EXPECT_EQ(true, cache.Has(2));
  EXPECT_EQ(true, cache.Has(3));
  EXPECT_EQ(true, cache.Has(4));
  EXPECT_EQ(true, cache.Has(5));
  EXPECT_EQ(false, cache.Has(6));

  cache.Put(6);
  EXPECT_EQ(false, cache.Has(1));
  EXPECT_EQ(true, cache.Has(2));
  EXPECT_EQ(true, cache.Has(3));
  EXPECT_EQ(true, cache.Has(4));
  EXPECT_EQ(true, cache.Has(5));
  EXPECT_EQ(true, cache.Has(6));
}

TEST(LruSet, OverflowGet) {
  Lru cache(2);
  cache.Put(1);
  cache.Put(2);
  cache.Has(1);
  cache.Put(3);

  EXPECT_EQ(true, cache.Has(1));
  EXPECT_EQ(false, cache.Has(2));
  EXPECT_EQ(true, cache.Has(3));
}
