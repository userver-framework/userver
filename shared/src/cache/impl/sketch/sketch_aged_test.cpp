#include <gtest/gtest.h>

#include <userver/cache/impl/sketch/aged.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using Type = double;
using Hash = std::hash<Type>;
using SimpleSketch =
    cache::impl::sketch::Sketch<Type, Hash, cache::impl::sketch::Policy::Bloom>;
using AgedSketch =
    cache::impl::sketch::Sketch<Type, Hash, cache::impl::sketch::Policy::Aged>;
}  // namespace

TEST(CacheSketch, AgedReset) {
  SimpleSketch simpleCounters(32);
  AgedSketch agedCounters(32);

  // 15 * 32 + 15 * 32 = 960
  for (int i = 0; i < 959; i++) {
    simpleCounters.Increment(i);
    agedCounters.Increment(i);
  }
  bool was_reset = false;
  for (int i = 0; i < 959; i++) {
    was_reset |= agedCounters.Estimate(i) < simpleCounters.Estimate(i);
  }
  EXPECT_TRUE(was_reset);
}

TEST(CacheSketch, AgedHeavyHitters) {
  AgedSketch counter(512, Hash{});
  for (int i = 100; i < 100'000; i++) {
    counter.Increment(static_cast<double>(i));
  }
  for (int i = 0; i < 10; i += 2) {
    for (int j = 0; j < i; j++) {
      counter.Increment(static_cast<double>(i));
    }
  }

  // A perfect popularity count yields an array [0, 0, 2, 0, 4, 0, 6, 0, 8, 0]
  std::vector<int> popularity(10);
  for (int i = 0; i < 10; i++) {
    popularity[i] = counter.Estimate(static_cast<double>(i));
  }
  EXPECT_TRUE(popularity[2] <= popularity[4]);
  EXPECT_TRUE(popularity[4] <= popularity[6]);
  EXPECT_TRUE(popularity[6] <= popularity[8]);
  for (int i = 0; i < 10; i++)
    if ((i == 0) || (i == 1) || (i == 3) || (i == 5) || (i == 7) || (i == 9)) {
      EXPECT_TRUE(popularity[i] <= popularity[2]);
    }
}

USERVER_NAMESPACE_END
