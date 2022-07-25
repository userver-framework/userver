#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <type_traits>

#include <userver/utils/assert.hpp>

/// @file userver/utils/rand.hpp
/// @brief Random number generators for use in a coroutine environment

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Virtualized standard UniformRandomBitGenerator concept, for use
/// with random number distributions
class RandomBase {
 public:
  using result_type = std::uint32_t;

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
/// @note @p from_inclusive must be less than @p to_exclusive
template <typename T>
T RandRange(T from_inclusive, T to_exclusive) {
  UINVARIANT(from_inclusive < to_exclusive,
             "Attempt to get a random value in an invalid range");
  if constexpr (std::is_floating_point_v<T>) {
    return std::uniform_real_distribution<T>{from_inclusive,
                                             to_exclusive}(DefaultRandom());
  } else {
    return std::uniform_int_distribution<T>{from_inclusive,
                                            to_exclusive - 1}(DefaultRandom());
  }
}

/// @brief Generates a random number in range [0, to)
/// @note The used random generator is not cryptographically secure
/// @note @p to_exclusive must be positive
template <typename T>
T RandRange(T to_exclusive) {
  return RandRange(T{0}, to_exclusive);
}

/// @brief Generate a random number in the whole `uint32_t` range
/// @note The used random generator is not cryptographically secure
/// @warning Don't use `Rand() % N`, use utils::RandRange instead
std::uint32_t Rand();

namespace impl {
std::uint64_t WeakRand64() noexcept;
}  // namespace impl

/// @brief Generates a random number in range [from, to)
/// @warning Produces insecure, easily predictable, biased and generally garbage
/// randomness, which is good enough for load balancing. Prefer utils::RandRange
/// outside of tight loops where you know you need that extra bit of unsafe
/// performance.
/// @note @p from_inclusive must be less than @p to_exclusive
template <typename T>
T WeakRandRange(T from_inclusive, T to_exclusive) noexcept {
  UASSERT_MSG(from_inclusive < to_exclusive,
              "Attempt to get a random value in an invalid range");
  static_assert(std::is_integral_v<T>);
  static_assert(sizeof(T) <= 8);
  UASSERT_MSG(static_cast<std::uint64_t>(to_exclusive - from_inclusive) <=
                  (std::numeric_limits<std::uint64_t>::max() >> 7),
              "WeakRandRange is highly biased for very large ranges, use "
              "RandRange instead");

  return static_cast<T>(from_inclusive +
                        impl::WeakRand64() % (to_exclusive - from_inclusive));
}

/// @brief Generates a random number in range [0, to)
/// @warning Produces insecure, easily predictable, biased and generally garbage
/// randomness, which is good enough for load balancing. Prefer utils::RandRange
/// outside of tight loops where you know you need that extra bit of unsafe
/// performance.
/// @note @p to_exclusive must be positive
template <typename T>
T WeakRandRange(T to_exclusive) noexcept {
  return WeakRandRange(T{0}, to_exclusive);
}

}  // namespace utils

USERVER_NAMESPACE_END
