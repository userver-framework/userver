#if 0

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
  AgedSketch agedCounters(32);
  for (std::size_t i = 0; i < 320 - 16; ++i) {
    agedCounters.Increment(static_cast<double>(i));
  }
  for (std::size_t i = 0; i < 16; ++i) {
    agedCounters.Increment(1.0);
  }

  EXPECT_TRUE(agedCounters.Estimate(1.0) != 16);
}

TEST(CacheSketch, AgedHeavyHitters) {
  AgedSketch counter(512, Hash{});
  for (int i = 100; i < 100'000; i++) {
    counter.Increment(static_cast<double>(i));
  }

  constexpr std::size_t kIterCount = 8;
  for (int i = 0; i < 10; i += 2) {
    for (std::size_t j = 0; j < kIterCount; ++j) {
      counter.Increment(static_cast<double>(i));
    }
  }

  // A perfect popularity count yields an array [8, 0, 8, 0, 8, 0, 8, 0, 8, 0]
  std::vector<int> popularity(10);
  for (int i = 0; i < 10; i++) {
    popularity[i] = counter.Estimate(static_cast<double>(i));
  }
  for (int even = 0; even < 10; even += 2) {
    for (int odd = 1; odd < 10; odd += 2) {
      EXPECT_TRUE(popularity[even] >= popularity[odd]);
    }
  }
}

USERVER_NAMESPACE_END

#endif
