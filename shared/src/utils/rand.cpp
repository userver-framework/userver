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

}  // namespace

RandomBase& DefaultRandom() {
  thread_local RandomImpl random;
  return random;
}

RandomBase& impl::DefaultRandomForHashSeed() {
  thread_local RandomImpl random;
  return random;
}

uint32_t Rand() {
  return std::uniform_int_distribution<uint32_t>{0}(DefaultRandom());
}

}  // namespace utils

USERVER_NAMESPACE_END
