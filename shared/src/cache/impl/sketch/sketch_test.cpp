#include <gtest/gtest.h>

#include <vector>

#include <userver/cache/impl/sketch/bloom.hpp>
#include <userver/cache/impl/sketch/doorkeeper.hpp>
#include "userver/cache/impl/sketch/policy.hpp"

USERVER_NAMESPACE_BEGIN

namespace {
using Type = double;
using Hash = std::hash<Type>;
template <typename T>
class CacheSketch : public ::testing::Test {
 public:
  using Sketch = cache::impl::sketch::Sketch<Type, Hash, T::value>;
};

using PolicyTypes =
    ::testing::Types<std::integral_constant<cache::impl::sketch::Policy,
                                            cache::impl::sketch::Bloom>,
                     std::integral_constant<cache::impl::sketch::Policy,
                                            cache::impl::sketch::Doorkeeper>>;

TYPED_TEST_SUITE(CacheSketch, PolicyTypes, );

}  // namespace

TYPED_TEST(CacheSketch, Estimate) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(512, Hash{});

  EXPECT_EQ(counter.Estimate(51), 0);
  counter.Increment(51);
  counter.Increment(51);

  counter.Increment(101);
  counter.Increment(101);

  EXPECT_TRUE(counter.Estimate(51) >= 2);
  EXPECT_TRUE(counter.Estimate(101) >= 2);
}

TYPED_TEST(CacheSketch, Increment) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(64, Hash{});

  for (int i = 0; i < 100; i++) counter.Increment(0);
  EXPECT_FALSE(counter.Increment(0));
  EXPECT_TRUE(counter.Estimate(0) >= 15);

  for (int i = 1; i < 64; i++) counter.Increment(i);

  int count = 0;
  for (int i = 1; i < 64; i++) count += counter.Estimate(i) >= 15;
  EXPECT_TRUE(count <= 4);
}

TYPED_TEST(CacheSketch, Reset) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(64, Hash{});

  counter.Increment(0);
  counter.Increment(0);
  counter.Increment(0);

  counter.Reset();
  EXPECT_TRUE(counter.Estimate(0) < 3);
}

TYPED_TEST(CacheSketch, Clear) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(64, Hash{});

  counter.Increment(0);
  counter.Increment(0);

  counter.Clear();
  EXPECT_EQ(counter.Estimate(0), 0);
}

TYPED_TEST(CacheSketch, SingleCounter) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(512, Hash{});

  for (int i = 0; i < 10; i++) counter.Increment(100);
  EXPECT_EQ(10, counter.Estimate(100));
}

TYPED_TEST(CacheSketch, ResetFull) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(64, Hash{});

  bool was_reset = true;
  for (int i = 0; i <= 1000 * 64; i++) counter.Increment(i);
  counter.Reset();
  for (int i = 0; i <= 64; i++)
    if (counter.Estimate(i) != 7) was_reset = false;

  EXPECT_TRUE(was_reset);
}

TYPED_TEST(CacheSketch, Full) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(16, Hash{});

  for (int i = 0; i < 100'000; i++) counter.Increment(i);

  bool actual = true;
  for (int i = 0; i < 100'000; i++) actual &= (counter.Estimate(i) >= 15);
  EXPECT_TRUE(actual);
}

TYPED_TEST(CacheSketch, HeavyHitters) {
  using Sketch = typename TestFixture::Sketch;
  Sketch counter(64, Hash{});
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
