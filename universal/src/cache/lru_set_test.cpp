#include <gtest/gtest.h>

#include <userver/cache/lru_set.hpp>

USERVER_NAMESPACE_BEGIN

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

TEST(LruSet, VisitAll) {
  Lru cache(10);
  cache.Put(1);
  cache.Put(2);
  cache.Put(3);

  std::size_t sum_keys = 0;
  // mutable - to test VisitAll compilation with non-const operator()
  cache.VisitAll([&sum_keys](const auto& key) mutable { sum_keys += key; });

  EXPECT_EQ(6, sum_keys);
}

TEST(LruSet, GetLeastUsed) {
  Lru cache(2);
  EXPECT_EQ(cache.GetLeastUsed(), nullptr);
  cache.Put(1);
  cache.Put(2);
  EXPECT_EQ(*cache.GetLeastUsed(), 1);
  cache.Has(1);
  EXPECT_EQ(*cache.GetLeastUsed(), 2);
}

USERVER_NAMESPACE_END
