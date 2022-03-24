#include <userver/utest/utest.hpp>

#include <userver/cache/nway_lru_cache.hpp>

USERVER_NAMESPACE_BEGIN

using Cache = cache::NWayLRU<int, int>;

UTEST(NWayLRU, Ctr) {
  UEXPECT_NO_THROW(Cache(1, 10));
  UEXPECT_NO_THROW(Cache(10, 10));
  UEXPECT_THROW(Cache(0, 10), std::logic_error);
}

UTEST(NWayLRU, Set) {
  Cache cache(1, 1);
  EXPECT_EQ(0, cache.GetSize());

  cache.Put(1, 1);
  EXPECT_EQ(1, cache.GetSize());

  cache.Put(2, 2);

  EXPECT_EQ(2, cache.Get(2));
  EXPECT_EQ(1, cache.GetSize());
  EXPECT_FALSE(cache.Get(1).has_value());
}

UTEST(NWayLRU, GetExpired) {
  Cache cache(1, 2);
  cache.Put(1, 1);
  cache.Put(2, 2);

  EXPECT_EQ(1, cache.Get(1));
  EXPECT_EQ(2, cache.GetSize());

  EXPECT_FALSE(cache.Get(1, [](int) { return false; }).has_value());
  EXPECT_EQ(1, cache.GetSize());

  EXPECT_FALSE(cache.Get(2, [](int) { return false; }).has_value());
  EXPECT_EQ(0, cache.GetSize());

  EXPECT_FALSE(cache.Get(1).has_value());
  EXPECT_EQ(0, cache.GetSize());
}

UTEST(NWayLRU, SetMultipleWays) {
  Cache cache(2, 1);
  cache.Put(1, 1);
  cache.Put(2, 2);

  EXPECT_EQ(2, cache.GetSize());
  EXPECT_EQ(2, cache.Get(2));
  EXPECT_EQ(1, cache.Get(1));
}

USERVER_NAMESPACE_END
