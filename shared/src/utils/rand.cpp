#include <userver/utils/rand.hpp>

#include <array>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

// 256 bits of randomness is enough for everyone
constexpr std::size_t kRandomSeedInts = 8;

class RandomImpl final : public RandomBase {
 public:
  // NOLINTNEXTLINE(cert-msc51-cpp)
  RandomImpl() {
    std::random_device device;

    std::array<std::seed_seq::result_type, kRandomSeedInts> random_chunks{};
    for (auto& random_chunk : random_chunks) {
      random_chunk = device();
    }

    std::seed_seq seed(random_chunks.begin(), random_chunks.end());
    gen_.seed(seed);
  }

  result_type operator()() override { return gen_(); }

 private:
  std::mt19937 gen_;
};

// Uses 64-bit xorshift algorithm, as described in
//
// Marsaglia, George (July 2003), "Xorshift RNGs".
// Journal of Statistical Software. 8 (14).
//
// (A TLDR can be found on Wikipedia.)
// The algorithm itself has good statistical properties, but it is easily
// predictable due to a small state size. And then we do `% size`, adding bias.
class WeakRandom final {
 public:
  using result_type = std::uint64_t;

  WeakRandom() noexcept
      : state_(std::uniform_int_distribution<result_type>{}(
            utils::DefaultRandom())) {}

  static constexpr result_type min() noexcept {
    return std::numeric_limits<result_type>::min();
  }

  static constexpr result_type max() noexcept {
    return std::numeric_limits<result_type>::max();
  }

  result_type operator()() noexcept {
    result_type x = state_;

    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;

    state_ = x;
    return x;
  }

 private:
  result_type state_;
};

WeakRandom& GetWeakRandom() noexcept {
  thread_local WeakRandom random;
  return random;
}

}  // namespace

RandomBase& DefaultRandom() {
  thread_local RandomImpl random;
  return random;
}

std::uint32_t Rand() {
  return std::uniform_int_distribution<std::uint32_t>{0}(DefaultRandom());
}

namespace impl {

std::uint64_t WeakRand64() noexcept { return GetWeakRandom()(); }

}  // namespace impl

}  // namespace utils

USERVER_NAMESPACE_END
