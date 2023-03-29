#if 0

#include <gtest/gtest.h>

#include <userver/cache/impl/window_tiny_lfu.hpp>

USERVER_NAMESPACE_BEGIN

using WTinyLFU =
    cache::impl::LruBase<int, int, std::hash<int>, std::equal_to<>,
                         cache::CachePolicy::kWTinyLFU>;


TODO: don't depend on SLRU probation_part which is 0.8 by default
TEST(WTinyLFU, PutNew) {
  WTinyLFU cache(100, std::hash<int>{}, std::equal_to<int>{}, 0.03);
  // bad
  const int main_size_lower_bound = std::round(97 * 0.8);

  int count_putted = 0;
  for (int i = 0; i < 3; i++) count_putted += cache.Put(i, i);
  EXPECT_EQ(3, count_putted);
  EXPECT_EQ(3, cache.GetSize());
  EXPECT_EQ(3, cache.GetWindowSize());

  count_putted = 0;
  for (int i = 3; i < 100; i++) count_putted += cache.Put(i, i);
  EXPECT_TRUE(count_putted >= main_size_lower_bound);

  EXPECT_TRUE(cache.Put(100, 100));

  EXPECT_EQ(3, cache.GetWindowSize());
  EXPECT_TRUE(cache.GetMainSize() >= main_size_lower_bound);
  EXPECT_TRUE(cache.Get(100));
  EXPECT_TRUE(cache.Get(99));
  EXPECT_TRUE(cache.Get(98));
  int count = 0;
  for (int i = 0; i < 100; i++)
    if (cache.Get(i)) count++;
  EXPECT_EQ(80, count);
}

TEST(WTinyLFU, PutAlreadyInWindow) {
  WTinyLFU cache(100, std::hash<int>{}, std::equal_to{}, 0.03);
  for (int i = 0; i < 100; i++) cache.Put(i, i);

  int count_putted = 0;
  for (int i = 97; i < 100; i++) {
    count_putted += cache.Put(i, 100);
    auto* res = cache.Get(i);
    EXPECT_TRUE(res);
    EXPECT_EQ(100, *res);
  }
  EXPECT_EQ(0, count_putted);
}

TEST(WTinyLFU, PutAlreadyInMain) {
  WTinyLFU cache(100, std::hash<int>{}, std::equal_to<int>{}, 0.03);
  for (int i = 0; i < 100; i++) cache.Put(i, i);

  // For SLRU engine only
  EXPECT_TRUE(cache.Put(5, 5));
  EXPECT_FALSE(cache.Put(20, 20));
  EXPECT_FALSE(cache.Put(37, 37));
  //

  auto* res = cache.Get(20);
  EXPECT_TRUE(res);
  EXPECT_EQ(20, *res);
  res = cache.Get(5);
  EXPECT_TRUE(res);
  EXPECT_EQ(5, *res);
  res = cache.Get(37);
  EXPECT_TRUE(res);
  EXPECT_EQ(37, *res);
}

TEST(WTinyLFU, Get) {
  WTinyLFU cache(100, std::hash<int>{}, std::equal_to{}, 0.03);
  for (int i = 0; i < 100; i++) cache.Put(i, i);
  auto* res = cache.Get(98);
  EXPECT_TRUE(res);
  EXPECT_EQ(98, *res);
  res = cache.Get(20);
  EXPECT_TRUE(res);
  EXPECT_EQ(20, *res);
}

TEST(WTinyLFU, Erase) {
  WTinyLFU cache(100, std::hash<int>{}, std::equal_to{}, 0.03);
  for (int i = 0; i < 100; i++) cache.Put(i, i);
  cache.Erase(98);
  auto* res = cache.Get(98);
  EXPECT_FALSE(res);
  cache.Erase(20);
  res = cache.Get(20);
  EXPECT_FALSE(res);
}

USERVER_NAMESPACE_END

#endif