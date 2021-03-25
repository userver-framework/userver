#pragma once

#include <cstdint>
#include <random>

/// @file utils/rand.hpp
/// @brief Random number generators for use in a coroutine environment

namespace utils {

/// @brief Virtualized standard UniformRandomBitGenerator concept, for use
/// with random number distributions
class RandomBase {
 public:
  using result_type = uint32_t;

  virtual ~RandomBase() = default;

  virtual result_type operator()() = 0;

  static constexpr result_type min() { return std::mt19937::min(); }
  static constexpr result_type max() { return std::mt19937::max(); }
};

/// @brief Returns a thread-local UniformRandomBitGenerator
/// @note The provided `Random` instance is not cryptographically secure
/// @warning Don't pass the returned `Random` across thread boundaries
RandomBase& DefaultRandom();

/// @brief Generates a random number in range [from, to)
/// @note The used random generator is not cryptographically secure
template <typename T, typename U>
U RandRange(T from_inclusive, U to_exclusive) {
  if constexpr (std::is_floating_point_v<U>) {
    return std::uniform_real_distribution<U>{from_inclusive,
                                             to_exclusive}(DefaultRandom());
  } else {
    return std::uniform_int_distribution<U>{from_inclusive,
                                            to_exclusive - 1}(DefaultRandom());
  }
}

/// @brief Generates a random number in range [0, to)
/// @note The used random generator is not cryptographically secure
template <typename T>
T RandRange(T to_exclusive) {
  return RandRange(0, to_exclusive);
}

/// @brief Generate a random number in the whole `uint32_t` range
/// @note The used random generator is not cryptographically secure
/// @warning Don't use `Rand() % N`, use `RandRange` instead
uint32_t Rand();

}  // namespace utils
