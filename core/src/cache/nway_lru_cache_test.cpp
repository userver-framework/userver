#include <utest/utest.hpp>

#include <cache/nway_lru_cache.hpp>

using Cache = cache::NWayLRU<int, int>;

TEST(NWayLRU, Ctr) {
  RunInCoro([]() {
    EXPECT_NO_THROW(Cache(1, 10));
    EXPECT_NO_THROW(Cache(10, 10));
    EXPECT_THROW(Cache(0, 10), std::logic_error);
  });
}

TEST(NWayLRU, Set) {
  RunInCoro([]() {
    Cache cache(1, 1);
    cache.Put(1, 1);
    cache.Put(2, 2);

    EXPECT_EQ(2, cache.Get(2));
    EXPECT_EQ(boost::none, cache.Get(1));
  });
}

TEST(NWayLRU, SetMultipleWays) {
  RunInCoro([]() {
    Cache cache(2, 1);
    cache.Put(1, 1);
    cache.Put(2, 2);

    EXPECT_EQ(2, cache.Get(2));
    EXPECT_EQ(1, cache.Get(1));
  });
}
