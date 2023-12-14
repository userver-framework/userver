#include <userver/utils/rand.hpp>

#include <random>
#include <type_traits>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr int kIterations = 1000;
}

TEST(Random, WithDefaultRandomDistribution) {
  for (int iter = 0; iter < kIterations; ++iter) {
    /// [WithDefaultRandom distribution]
    std::uniform_int_distribution<std::uint64_t> distribution{1, 6};
    const auto x = utils::WithDefaultRandom(distribution);
    /// [WithDefaultRandom distribution]
    EXPECT_GE(x, 1);
    EXPECT_LE(x, 6);
  }
}

TEST(Random, WithDefaultRandomMultiple) {
  /// [WithDefaultRandom multiple]
  std::vector<std::uint64_t> random_numbers;
  random_numbers.reserve(kIterations);

  // utils::WithDefaultRandom induces some overhead to ensure coroutine-safety.
  // In a performance-critical code section, a single WithDefaultRandom call is
  // more efficient than multiple `WithDefaultRandom(distribution)`.
  utils::WithDefaultRandom([&](utils::RandomBase& rng) {
    std::uniform_int_distribution<std::uint64_t> distribution{1, 6};
    for (int iter = 0; iter < kIterations; ++iter) {
      random_numbers.push_back(distribution(rng));
    }
  });
  /// [WithDefaultRandom multiple]

  EXPECT_EQ(random_numbers.size(), kIterations);
  for (const auto x : random_numbers) {
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

TEST(Random, Shuffle) {
  std::vector<int> nums{100};
  std::iota(nums.begin(), nums.end(), 1);

  auto permutation = nums;
  utils::Shuffle(permutation);

  EXPECT_TRUE(
      std::is_permutation(nums.begin(), nums.end(), permutation.begin()));
}

USERVER_NAMESPACE_END
