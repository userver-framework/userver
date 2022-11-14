#include <gtest/gtest.h>

#include <vector>

#include <userver/cache/impl/frequency_sketch.hpp>

USERVER_NAMESPACE_BEGIN

using cache::impl::FrequencySketch;

TEST(Bloom, Reset) {
  FrequencySketch<int, cache::FrequencySketchPolicy::Bloom> bloom(64);

  bool was_reset = false;
  for (int i = 0; i <= 1000 * 64; i++) {
    bloom.RecordAccess(i);
    if (bloom.Size() != i) {
      was_reset = true;
      break;
    }
  }

  EXPECT_TRUE(was_reset);
}

TEST(Bloom, Full) {
  FrequencySketch<int, cache::FrequencySketchPolicy::Bloom> bloom(16, 1000);

  for (int i = 0; i < 100'000; i++)
    bloom.RecordAccess(i);

  for (int i = 0; i < 100'000; i++)
    EXPECT_EQ(bloom.GetFrequency(i), 15);
}

TEST(Bloom, HeavyHitters) {
  FrequencySketch<double, cache::FrequencySketchPolicy::Bloom> bloom(512);
  for (int i = 100; i < 100'000; i++) {
    bloom.RecordAccess(static_cast<double>(i));
  }
  for (int i = 0; i < 10; i += 2) {
    for (int j = 0; j < i; j++) {
      bloom.RecordAccess(static_cast<double>(i));
    }
  }

  // A perfect popularity count yields an array [0, 0, 2, 0, 4, 0, 6, 0, 8, 0]
  auto* popularity = new int[10];
  for (int i = 0; i < 10; i++) {
    popularity[i] = bloom.GetFrequency(static_cast<double>(i));
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
