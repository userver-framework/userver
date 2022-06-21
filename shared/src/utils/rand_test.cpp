#include <userver/utils/rand.hpp>

#include <random>
#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr int kIterations = 1000;
}

TEST(Random, Distributions) {
  std::uniform_int_distribution distribution{1, 6};
  for (int iter = 0; iter < kIterations; ++iter) {
    const auto x = distribution(utils::DefaultRandom());
    EXPECT_GE(x, 1);
    EXPECT_LE(x, 6);
  }
}

TEST(Random, RandRange) {
  for (int iter = 0; iter < kIterations; ++iter) {
    const auto x = utils::RandRange(uint64_t{1}, uint64_t{7});
    static_assert(std::is_same_v<decltype(x), const uint64_t>);
    EXPECT_GE(x, 1);
    EXPECT_LE(x, 6);
  }

  for (int iter = 0; iter < kIterations; ++iter) {
    const auto x = utils::RandRange(3.6);
    static_assert(std::is_same_v<decltype(x), const double>);
    EXPECT_GE(x, 0);
    EXPECT_LE(x, 3.6);
  }
}

USERVER_NAMESPACE_END
