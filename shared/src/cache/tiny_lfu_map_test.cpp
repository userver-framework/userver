#include <gtest/gtest.h>

#include <unordered_map>

#include <userver/cache/impl/hash.hpp>
#include <userver/cache/impl/sketch/policy.hpp>
#include <userver/cache/impl/tiny_lfu.hpp>
#include <userver/utils/rand.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::sketch {
template <typename T, typename Hash>
class Sketch<T, Hash, Policy::Trivial> {
 public:
  explicit Sketch(size_t, const Hash& = Hash{}) {}
  int Estimate(const T& key) {
    std::cout << key << ' ' << counters_[key] << std::endl;
    return counters_[key];
  }
  bool Increment(const T& key) {
    counters_[key]++;
    return true;
  }

 private:
  std::unordered_map<T, int> counters_;
};
}  // namespace cache::impl::sketch

namespace {

using Type = int;
using Hash = std::hash<Type>;
using Equal = std::equal_to<Type>;
using Cache =
    cache::impl::TinyLfu<Type, int, Hash, Equal, cache::CachePolicy::kLRU,
                         std::hash<Type>, cache::impl::sketch::Policy::Trivial>;

template <typename Cache>
std::vector<int> Get(Cache& cache) {
  std::vector<int> actual;
  cache.VisitAll([&actual](int key, int) { actual.push_back(key); });
  std::sort(actual.begin(), actual.end());
  return actual;
}
}  // namespace

TEST(TinyLfu, PutGet) {
  Cache cache(256, Hash{}, std::equal_to<int>{});
  int count = 0;
  for (int i = 0; i < 256; i++) count += cache.Put(i, i);
  EXPECT_EQ(count, 256);
  count = 0;
  for (int i = 0; i < 256; i++) count += (cache.Get(i) ? 1 : 0);
  EXPECT_EQ(count, 256);

  int try_count = 0;
  for (int i = 0; i < 20; i++, try_count++) {
    cache.Put(256, 256);
    auto* res = cache.Get(256);
    if (res) break;
  }
  EXPECT_EQ(try_count, 1);
}

TEST(TinyLfu, PutUpdate) {
  Cache cache(256, Hash{}, std::equal_to<int>{});
  // Fill
  for (int i = 1; i <= 100000; i++) {
    int key = utils::RandRange(1, 257);
    cache.Put(key, i);
  }

  cache.Put(143, 0);
  auto* res = cache.Get(143);
  EXPECT_TRUE(res);
  if (res) {
    EXPECT_EQ(*res, 0);
  }
}

TEST(TinyLfu, Get) {
  Cache cache(256, Hash{}, std::equal_to<int>{});
  cache.Put(0, 0);
  cache.Put(1, 1);

  auto* res = cache.Get(0);
  EXPECT_TRUE(res);
  if (res) {
    EXPECT_EQ(*res, 0);
  }

  res = cache.Get(2);
  EXPECT_FALSE(res);

  for (int i = 2; i < 256; i++) cache.Put(i, i);

  EXPECT_FALSE(cache.Get(257));
  EXPECT_FALSE(cache.Get(257));
  cache.Put(257, 257);
  res = cache.Get(257);
  EXPECT_TRUE(res);
  if (res) {
    EXPECT_EQ(*res, 257);
  }
}

TEST(TinyLfu, Size2) {
  Cache cache(2, Hash{}, std::equal_to<int>{});
  cache.Put(1, 1);
  cache.Put(2, 2);
  EXPECT_TRUE(cache.Get(1));
  EXPECT_TRUE(cache.Get(2));
  EXPECT_EQ(2, cache.GetSize());
}

USERVER_NAMESPACE_END