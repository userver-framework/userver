#include <userver/utils/filter_bloom.hpp>

#include <string>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(FilterBloom, Sample) {
  /// [Sample filter bloom usage]
  utils::FilterBloom<int> filter_bloom(16 * 1024);
  for (int i = 0; i < 512; ++i) {
    for (std::size_t j = 0; j < 5; ++j) {
      filter_bloom.Increment(i);
    }
  }
  for (int i = 512; i < 1024; ++i) {
    for (std::size_t j = 0; j < 10; ++j) {
      filter_bloom.Increment(i);
    }
  }

  for (int i = 0; i < 512; ++i) {
    EXPECT_GE(7, filter_bloom.Estimate(i));
  }
  for (int i = 512; i < 1024; ++i) {
    EXPECT_LE(10, filter_bloom.Estimate(i));
  }
  /// [Sample filter bloom usage]
}

TEST(FilterBloom, StringHash) {
  utils::FilterBloom<std::string> filter{};
  std::string str;
  for (std::size_t i = 0; i < 10; ++i) {
    str += 'A';
    filter.Increment(str);
  }
  for (std::size_t i = 0; i < 10; ++i) {
    EXPECT_EQ(true, filter.Has(str));
    str.pop_back();
  }
}

TEST(FilterBloom, ApproximateCount) {
  utils::FilterBloom<double> filter(200);
  double value = 1.0;
  for (std::size_t i = 0; i < 20; ++i) {
    filter.Increment(value);
    value += 0.1;
  }
  for (std::size_t i = 0; i < 20; ++i) {
    value -= 0.1;
    std::size_t count = filter.Estimate(value);
    EXPECT_LE(1, count);
    EXPECT_GE(3, count);
  }
}

TEST(FilterBloom, SmallCounterSize) {
  utils::FilterBloom<std::string, uint8_t> filter(1000);
  std::string str;
  for (std::size_t i = 0; i < 10; ++i) {
    str.push_back('a');
    filter.Increment(str);
    filter.Increment(str);
  }
  for (std::size_t i = 0; i < 10; ++i) {
    EXPECT_LE(static_cast<uint8_t>(2), filter.Estimate(str));
    str.pop_back();
  }
}

TEST(FilterBloom, HasValue) {
  utils::FilterBloom<float, uint8_t> filter(32);
  for (std::size_t i = 0; i < 5; ++i) {
    filter.Increment(static_cast<float>(i) * static_cast<float>(i));
  }
  for (std::size_t i = 0; i < 5; ++i) {
    EXPECT_EQ(true, filter.Has(static_cast<float>(i) * static_cast<float>(i)));
  }
}

TEST(FilterBloom, Clear) {
  utils::FilterBloom<std::size_t, uint_fast16_t> filter(32);
  filter.Increment(1);
  filter.Increment(2);
  EXPECT_EQ(true, filter.Has(1));
  EXPECT_EQ(true, filter.Has(2));
  filter.Clear();
  EXPECT_EQ(false, filter.Has(1));
  EXPECT_EQ(false, filter.Has(2));
}

USERVER_NAMESPACE_END
