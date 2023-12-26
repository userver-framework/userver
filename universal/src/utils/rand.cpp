#include <userver/utils/rand.hpp>

#include <array>
#include <thread>

#include <userver/compiler/thread_local.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace {

template <typename T>
auto& AsLvalue(T&& rvalue) noexcept {
  return rvalue;
}

class RandomImpl final : public RandomBase {
 public:
  RandomImpl() : gen_(AsLvalue(impl::MakeSeedSeq())) {}

  result_type operator()() override { return gen_(); }

 private:
  std::mt19937 gen_;
};

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

RandomBase& GetDefaultRandom() {
  return compiler::impl::ThreadLocal([] { return RandomImpl{}; });
}

std::uintptr_t GetCurrentThreadId() noexcept {
  return compiler::impl::GetCurrentThreadIdDebug();
}

}  // namespace impl

std::uint32_t Rand() {
  return WithDefaultRandom(std::uniform_int_distribution<std::uint32_t>{0});
}

}  // namespace utils

USERVER_NAMESPACE_END
