#include <gtest/gtest.h>

#include <userver/cache/impl/tiny_lfu.hpp>

USERVER_NAMESPACE_BEGIN

template <typename T>
class TinyLFU : public ::testing::Test {
 public:
  using Hash = std::hash<int>;
  using Equal = std::equal_to<int>;
  using Cache =
      cache::impl::LruBase<int, int, Hash, Equal,
                           cache::CachePolicy::kTinyLFU, T::value>;
};

using InnerPolicyTypes = ::testing::Types<
    std::integral_constant<cache::CachePolicy, cache::CachePolicy::kLRU>,
    std::integral_constant<cache::CachePolicy, cache::CachePolicy::kSLRU>>;

TYPED_TEST_SUITE(TinyLFU, InnerPolicyTypes, );

template <typename Cache>
std::vector<int> Get(Cache& cache) {
  std::vector<int> actual;
  cache.VisitAll([&actual](int key, int) { actual.push_back(key); });
  std::sort(actual.begin(), actual.end());
  return actual;
}

TYPED_TEST(TinyLFU, PutGet) {
  using Cache = typename TestFixture::Cache;
  Cache cache(2, std::hash<int>{}, std::equal_to<int>{});
  EXPECT_TRUE(cache.Put(0, 0));
  cache.Get(0);
  EXPECT_TRUE(cache.Put(1, 1));
  cache.Get(1);
  cache.Get(1);

  EXPECT_EQ(Get(cache), (std::vector<int>{0, 1}));
  int try_count = 0;
  for (int i = 0; i < 10; i++) {
    try_count++;
    EXPECT_TRUE(cache.Put(2, 2));
    if (Get(cache) == (std::vector<int>{1, 2})) break;
  }
  EXPECT_TRUE(try_count >= 2 && try_count < 10);

  EXPECT_EQ(Get(cache), (std::vector<int>{1, 2}));
  try_count = 0;
  for (int i = 0; i < 10; i++) {
    try_count++;
    EXPECT_TRUE(cache.Put(3, 3));
    if (Get(cache) == (std::vector<int>{2, 3})) break;
  }
  EXPECT_TRUE(try_count >= 3 && try_count < 10);
}

TYPED_TEST(TinyLFU, EmplaceGet) {
  using Cache = typename TestFixture::Cache;
  Cache cache(2, std::hash<int>{}, std::equal_to<int>{});
  auto* res = cache.Emplace(0, 0);
  EXPECT_TRUE(res);
  EXPECT_EQ(*res, 0);
  cache.Get(0);

  res = cache.Emplace(1, 1);
  EXPECT_TRUE(res);
  EXPECT_EQ(*res, 1);
  cache.Get(1);
  cache.Get(1);

  EXPECT_EQ(Get(cache), (std::vector<int>{0, 1}));
  int try_count = 0;
  for (int i = 0; i < 10; i++) {
    try_count++;
    res = cache.Emplace(2, 2);
    if (Get(cache) == (std::vector<int>{1, 2})) break;
    EXPECT_FALSE(res);
  }
  EXPECT_TRUE(try_count >= 2 && try_count < 10);
  EXPECT_EQ(*res, 2);

  EXPECT_EQ(Get(cache), (std::vector<int>{1, 2}));
  try_count = 0;
  for (int i = 0; i < 10; i++) {
    try_count++;
    res = cache.Emplace(3, 3);
    if (Get(cache) == (std::vector<int>{2, 3})) break;
    EXPECT_FALSE(res);
  }
  EXPECT_TRUE(try_count >= 3 && try_count < 10);
  EXPECT_EQ(*res, 3);
}

TYPED_TEST(TinyLFU, Get) {
  using Cache = typename TestFixture::Cache;
  Cache cache(2, std::hash<int>{}, std::equal_to<int>{});
  cache.Put(0, 0);
  cache.Put(1, 1);

  auto* res = cache.Get(0);
  EXPECT_TRUE(res);
  EXPECT_EQ(*res, 0);

  res = cache.Get(0);
  EXPECT_TRUE(res);
  EXPECT_EQ(*res, 0);

  int try_count = 0;
  for (int i = 0; i < 10; i++) {
    try_count++;
    cache.Put(2, 2);
    if (Get(cache) == (std::vector<int>{0, 2})) break;
  }
  EXPECT_TRUE(try_count >= 1 && try_count < 10);

  try_count = 0;
  for (int i = 0; i < 10; i++) {
    try_count++;
    cache.Put(3, 3);
    if (Get(cache) == (std::vector<int>{2, 3})) break;
  }
  EXPECT_TRUE(try_count >= 3 && try_count < 10);

  res = cache.Get(2);
  EXPECT_TRUE(res);
  EXPECT_EQ(*res, 2);
  res = cache.Get(3);
  EXPECT_TRUE(res);
  EXPECT_EQ(*res, 3);
}

TYPED_TEST(TinyLFU, Size2) {
  using Cache = typename TestFixture::Cache;
  Cache cache(2, std::hash<int>{}, std::equal_to<int>{});
  cache.Put(1, 1);
  cache.Put(2, 2);
  EXPECT_TRUE(cache.Get(1));
  EXPECT_TRUE(cache.Get(2));
  EXPECT_EQ(2, cache.GetSize());
}

USERVER_NAMESPACE_END