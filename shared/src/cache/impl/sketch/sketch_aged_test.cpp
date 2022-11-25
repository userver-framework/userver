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

USERVER_NAMESPACE_END
