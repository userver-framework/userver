#include <userver/utils/rand.hpp>

#include <array>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

compiler::ThreadLocal local_random_impl = [] { return impl::RandomImpl{}; };

}  // namespace

namespace impl {

std::seed_seq MakeSeedSeq() {
    // 256 bits of randomness is enough for everyone
    constexpr std::size_t kRandomSeedInts = 8;

    std::random_device device;

    std::array<std::seed_seq::result_type, kRandomSeedInts> random_chunks{};
    for (auto& random_chunk : random_chunks) {
        random_chunk = device();
    }

    return std::seed_seq(random_chunks.begin(), random_chunks.end());
}

RandomImpl::RandomImpl()
    : gen_([] {
          auto seed_seq = MakeSeedSeq();
          return decltype(gen_){seed_seq};
      }())
{}

compiler::ThreadLocalScope<RandomImpl> UseLocalRandomImpl() { return local_random_impl.Use(); }

}  // namespace impl

std::uint32_t Rand() { return WithDefaultRandom(std::uniform_int_distribution<std::uint32_t>{0}); }

}  // namespace utils

USERVER_NAMESPACE_END
