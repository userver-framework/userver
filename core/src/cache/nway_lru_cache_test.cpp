#include <userver/utest/utest.hpp>

#include <type_traits>

#include <userver/cache/nway_lru_cache.hpp>

USERVER_NAMESPACE_BEGIN

template <typename T>
class NWayLRU : public ::testing::Test {
 public:
  using Cache = T;
};

using NWayLRUPolicyTypes = ::testing::Types<
    cache::NWayLRU<int, int, std::hash<int>, std::equal_to<int>, cache::CachePolicy::kLRU>,
    cache::NWayLRU<int, int, std::hash<int>, std::equal_to<int>, cache::CachePolicy::kSLRU>,
    cache::NWayLRU<int, int, std::hash<int>, std::equal_to<int>, cache::CachePolicy::kTinyLFU>>;

TYPED_UTEST_SUITE(NWayLRU, NWayLRUPolicyTypes);

TYPED_UTEST(NWayLRU, Ctr) {
  using Cache = typename TestFixture::Cache;
  UEXPECT_NO_THROW(Cache(1, 10));
  UEXPECT_NO_THROW(Cache(10, 10));
  UEXPECT_THROW(Cache(0, 10), std::logic_error);
}

TYPED_UTEST(NWayLRU, Set) {
  using Cache = typename TestFixture::Cache;
  Cache cache(1, 1);
  EXPECT_EQ(0, cache.GetSize());

  cache.Put(1, 1);
  EXPECT_EQ(1, cache.GetSize());

  cache.Put(2, 2);
  cache.Put(2, 2);

  EXPECT_EQ(2, cache.Get(2));
  EXPECT_EQ(1, cache.GetSize());
  EXPECT_FALSE(cache.Get(1).has_value());
}

TYPED_UTEST(NWayLRU, GetExpired) {
  using Cache = typename TestFixture::Cache;
  Cache cache(1, 2);
  cache.Put(1, 1);
  cache.Put(2, 2);

  EXPECT_EQ(1, cache.Get(1));
  EXPECT_EQ(2, cache.Get(2));
  EXPECT_EQ(2, cache.GetSize());

  EXPECT_FALSE(cache.Get(1, [](int) { return false; }).has_value());
  EXPECT_EQ(1, cache.GetSize());

  EXPECT_FALSE(cache.Get(2, [](int) { return false; }).has_value());
  EXPECT_EQ(0, cache.GetSize());

  EXPECT_FALSE(cache.Get(1).has_value());
  EXPECT_EQ(0, cache.GetSize());
}

TYPED_UTEST(NWayLRU, SetMultipleWays) {
  using Cache = typename TestFixture::Cache;
  Cache cache(2, 1);
  cache.Put(1, 1);
  cache.Put(2, 2);

  EXPECT_EQ(2, cache.GetSize());
  EXPECT_EQ(2, cache.Get(2));
  EXPECT_EQ(1, cache.Get(1));
}

USERVER_NAMESPACE_END
