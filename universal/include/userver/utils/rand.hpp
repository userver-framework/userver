#pragma once

/// @file userver/utils/rand.hpp
/// @brief Random number generators
/// @ingroup userver_universal

#include <algorithm>
#include <cstdint>
#include <limits>
#include <random>
#include <utility>

#include <userver/compiler/thread_local.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

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

namespace impl {

std::seed_seq MakeSeedSeq();

class RandomImpl final : public RandomBase {
 public:
  RandomImpl();

  result_type operator()() override { return gen_(); }

 private:
  std::mt19937 gen_;
};

compiler::ThreadLocalScope<RandomImpl> UseLocalRandomImpl();

}  // namespace impl

/// @brief Calls @a func with a thread-local UniformRandomBitGenerator
/// (specifically of type utils::RandomBase).
///
/// @note The provided `RandomBase` instance is not cryptographically secure.
///
/// @a func should not contain any task context switches. That is, it should not
/// use any userver synchronization primitives and should not access
/// userver-based web or database clients.
///
/// @a func should not store the reference to the provided thread-local variable
/// for use outside of the WithThreadLocal scope.
///
/// Prefer utils::RandRange if possible.
///
/// ## Usage example
///
/// Standard distributions can be passed to WithDefaultRandom directly:
/// @snippet utils/rand_test.cpp  WithDefaultRandom distribution
///
/// A lambda can be passed to perform a series of operations more efficiently:
/// @snippet utils/rand_test.cpp  WithDefaultRandom multiple
///
/// @param func functor that will be invoked with the RNG
/// @returns The invocation result of @a func
template <typename Func>
decltype(auto) WithDefaultRandom(Func&& func) {
  auto random = impl::UseLocalRandomImpl();
  return std::forward<Func>(func)(static_cast<RandomBase&>(*random));
}

/// @brief Generates a random number in range [from, to)
/// @note The used random generator is not cryptographically secure
/// @note `from_inclusive` must be less than `to_exclusive`
template <typename T>
T RandRange(T from_inclusive, T to_exclusive) {
  UINVARIANT(from_inclusive < to_exclusive,
             "attempt to get a random value in an incorrect range");
  if constexpr (std::is_floating_point_v<T>) {
    return utils::WithDefaultRandom(
        std::uniform_real_distribution<T>{from_inclusive, to_exclusive});
  } else {
    // 8-bit types are not allowed in uniform_int_distribution, so increase the
    // T size.
    return utils::WithDefaultRandom(
        std::uniform_int_distribution<std::common_type_t<T, unsigned short>>{
            from_inclusive, to_exclusive - 1});
  }
}

/// @brief Generates a random number in range [0, to)
/// @note The used random generator is not cryptographically secure
template <typename T>
T RandRange(T to_exclusive) {
  return RandRange(T{0}, to_exclusive);
}

/// @brief Shuffles the elements within the container
/// @note The method used for determining a random permutation is not
/// cryptographically secure
template <typename Container>
void Shuffle(Container& container) {
  utils::WithDefaultRandom([&container](RandomBase& rng) {
    std::shuffle(std::begin(container), std::end(container), rng);
  });
}

/// @brief Generate a random number in the whole `uint32_t` range
/// @note The used random generator is not cryptographically secure
/// @warning Don't use `Rand() % N`, use `RandRange` instead
std::uint32_t Rand();

}  // namespace utils

USERVER_NAMESPACE_END
