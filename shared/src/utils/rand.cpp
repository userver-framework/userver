#include <userver/utils/rand.hpp>

#include <array>

#include <boost/multiprecision/cpp_int.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

// 256 bits of randomness is enough for everyone
constexpr std::size_t kRandomSeedInts = 8;

class RandomImpl final : public RandomBase {
 public:
  // NOLINTNEXTLINE(cert-msc51-cpp)
  RandomImpl() noexcept {
    // TODO fall back on various sources of randomness in case
    //  std::random_device throws
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

// reference: Fast Random Integer Generation in an Interval
// ACM Transactions on Modeling and Computer Simulation 29 (1), 2019
// https://arxiv.org/abs/1805.10941
//
// Used by GCC, but not yet Clang:
// https://gcc.gnu.org/git/?p=gcc.git;a=commit;h=98c37d3bacbb2f8bbbe56ed53a9547d3be01b66b
template <typename DoubleWidth, typename UniformRandomBitGenerator,
          typename UInt>
UInt FastRandRange(UniformRandomBitGenerator& gen, UInt to_exclusive) noexcept {
  static_assert(std::is_unsigned_v<UInt>);
  static_assert(std::numeric_limits<DoubleWidth>::digits ==
                std::numeric_limits<UInt>::digits * 2);

  auto product =
      static_cast<DoubleWidth>(gen()) * static_cast<DoubleWidth>(to_exclusive);
  auto low = static_cast<UInt>(product);
  if (low < to_exclusive) {
    const UInt threshold = -to_exclusive % to_exclusive;
    while (low < threshold) {
      product = static_cast<DoubleWidth>(gen()) *
                static_cast<DoubleWidth>(to_exclusive);
      low = static_cast<UInt>(product);
    }
  }
  return static_cast<UInt>(product >> std::numeric_limits<UInt>::digits);
}

}  // namespace

RandomBase& DefaultRandom() noexcept {
  thread_local RandomImpl random;
  return random;
}

std::uint32_t Rand() noexcept {
  return std::uniform_int_distribution<std::uint32_t>{0}(DefaultRandom());
}

namespace impl {

std::uint32_t WeakRandRange(std::uint32_t to_exclusive) noexcept {
  return FastRandRange<std::uint64_t>(GetWeakRandom(), to_exclusive);
}

std::uint64_t WeakRandRange(std::uint64_t to_exclusive) noexcept {
  return FastRandRange<boost::multiprecision::uint128_t>(GetWeakRandom(),
                                                         to_exclusive);
}

}  // namespace impl

}  // namespace utils

USERVER_NAMESPACE_END
