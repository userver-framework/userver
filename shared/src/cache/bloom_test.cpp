#include <gtest/gtest.h>

#include <vector>

#include <userver/cache/impl/frequency_sketch.hpp>
#include <userver/cache/impl/hash.hpp>

USERVER_NAMESPACE_BEGIN

template <typename T>
class FrequencySketchF : public ::testing::Test {
 public:
  using Hash = cache::impl::tools::Jenkins<double>;
  using FrequencySketch = cache::impl::FrequencySketch<double, Hash, T::value>;
};

using PolicyTypes = ::testing::Types<
    std::integral_constant<cache::FrequencySketchPolicy,
                           cache::FrequencySketchPolicy::Bloom>,
    std::integral_constant<cache::FrequencySketchPolicy,
                           cache::FrequencySketchPolicy::DoorkeeperBloom>>;

TYPED_TEST_SUITE(FrequencySketchF, PolicyTypes, );

TYPED_TEST(FrequencySketchF, SingleCounter) {
  using FrequencySketch = typename TestFixture::FrequencySketch;
  using Hash = typename TestFixture::Hash;
  FrequencySketch counter(512, Hash{});

  for (int i = 0; i < 10; i++) counter.RecordAccess(100);
  EXPECT_EQ(10, counter.GetFrequency(100));
}

TYPED_TEST(FrequencySketchF, Reset) {
  using FrequencySketch = typename TestFixture::FrequencySketch;
  using Hash = typename TestFixture::Hash;
  FrequencySketch counter(64, Hash{});

  bool was_reset = false;
  for (int i = 0; i <= 1000 * 64; i++) {
    counter.RecordAccess(i);
    if (counter.Size() != i) {
      was_reset = true;
      break;
    }
  }

  EXPECT_TRUE(was_reset);
}

TEST(FrequencySketchF, Full) {
  using Hash = cache::impl::tools::Jenkins<double>;
  using FrequencySketch =
      cache::impl::FrequencySketch<double, Hash,
                                   cache::FrequencySketchPolicy::Bloom>;
  FrequencySketch counter(16, Hash{});

  for (int i = 0; i < 100'000; i++) counter.RecordAccess(i);

  for (int i = 0; i < 100'000; i++) EXPECT_EQ(counter.GetFrequency(i), 15);
}

TYPED_TEST(FrequencySketchF, HeavyHitters) {
  using FrequencySketch = typename TestFixture::FrequencySketch;
  using Hash = typename TestFixture::Hash;
  FrequencySketch counter(512, Hash{});
  for (int i = 100; i < 100'000; i++) {
    counter.RecordAccess(static_cast<double>(i));
  }
  for (int i = 0; i < 10; i += 2) {
    for (int j = 0; j < i; j++) {
      counter.RecordAccess(static_cast<double>(i));
    }
  }

  // A perfect popularity count yields an array [0, 0, 2, 0, 4, 0, 6, 0, 8, 0]
  std::vector<int> popularity(10);
  for (int i = 0; i < 10; i++) {
    popularity[i] = counter.GetFrequency(static_cast<double>(i));
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
