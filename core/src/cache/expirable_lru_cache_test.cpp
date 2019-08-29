#include <utest/utest.hpp>

#include <cache/expirable_lru_cache.hpp>
#include <utils/mock_now.hpp>

TEST(ExpirableLruCache, Hit) {
  RunInCoro([] {
    cache::ExpirableLruCache<int, int> cache(1, 1);
    int count = 0;

    EXPECT_EQ(0, cache.Get(1, [&count](int) {
      count++;
      return 0;
    }));
    EXPECT_EQ(1, count);

    EXPECT_EQ(0, cache.Get(1, [&count](int) {
      count++;
      return 0;
    }));
    EXPECT_EQ(1, count);
  });
}

TEST(ExpirableLruCache, Expire) {
  RunInCoro([] {
    cache::ExpirableLruCache<int, int> cache(1, 1);
    cache.UpdateMaxLifetime(std::chrono::seconds(2));

    int count = 0;
    utils::datetime::MockNowSet(std::chrono::system_clock::now());

    EXPECT_EQ(0, cache.Get(1, [&count](int) {
      count++;
      return 0;
    }));
    EXPECT_EQ(1, count);

    EXPECT_EQ(0, cache.Get(1, [&count](int) {
      count++;
      return 0;
    }));
    EXPECT_EQ(1, count);

    utils::datetime::MockSleep(std::chrono::seconds(3));
    EXPECT_EQ(0, cache.Get(1, [&count](int) {
      count++;
      return 0;
    }));
    EXPECT_EQ(2, count);
  });
}

TEST(ExpirableLruCache, DefaultNoExpire) {
  RunInCoro([] {
    cache::ExpirableLruCache<int, int> cache(1, 1);

    int count = 0;
    utils::datetime::MockNowSet(std::chrono::system_clock::now());

    EXPECT_EQ(0, cache.Get(1, [&count](int) {
      count++;
      return 0;
    }));
    EXPECT_EQ(1, count);

    for (int i = 0; i < 10; i++) {
      utils::datetime::MockSleep(std::chrono::seconds(2));
      EXPECT_EQ(0, cache.Get(1, [&count](int) {
        count++;
        return 1;
      }));
    }
    EXPECT_EQ(1, count);
  });
}

TEST(ExpirableLruCache, InvalidateByKey) {
  RunInCoro([] {
    cache::ExpirableLruCache<int, int> cache(1, 1);
    int count = 0;

    EXPECT_EQ(0, cache.Get(1, [&count](int) {
      count++;
      return 0;
    }));
    EXPECT_EQ(1, count);

    cache.InvalidateByKey(1);

    EXPECT_EQ(1, cache.Get(1, [&count](int) {
      count++;
      return 1;
    }));
    EXPECT_EQ(2, count);
  });
}
