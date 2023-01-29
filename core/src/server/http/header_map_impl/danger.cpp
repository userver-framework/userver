#include <server/http/header_map_impl/danger.hpp>

#include <cstring>

#include <userver/utils/assert.hpp>
#include <userver/utils/rand.hpp>
#include <userver/utils/str_icase.hpp>

#include <server/http/header_map_impl/header_name.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http::header_map_impl {

namespace {

inline std::size_t unaligned_load(const char* p) {
  std::size_t result{};
  std::memcpy(&result, p, sizeof(result));
  return result;
}

inline std::size_t load_bytes(const char* p, int n) {
  std::size_t result = 0;
  --n;

  do {
    result = (result << 8) + static_cast<unsigned char>(p[n]);
  } while (--n >= 0);
  return result;
}

inline std::size_t shift_mix(std::size_t v) { return v ^ (v >> 47); }

// MurMur2 implementation taken from
// https://github.com/gcc-mirror/gcc/blob/da3aca031be736fe4fa8daa57c7efa69dc767160/libstdc%2B%2B-v3/libsupc%2B%2B/hash_bytes.cc
class GccMurMurHasher final {
 public:
  GccMurMurHasher(std::size_t seed) : seed_{seed} {}

  std::size_t operator()(const void* ptr, std::size_t len) const {
    constexpr std::size_t mul = (0xc6a4a793UL << 32UL) + 0x5bd1e995UL;
    const char* const buf = static_cast<const char*>(ptr);

    const std::size_t len_aligned = len & ~0x7UL;
    const char* const end = buf + len_aligned;
    size_t hash = seed_ ^ (len * mul);
    for (const char* p = buf; p != end; p += 8) {
      const size_t data = shift_mix(unaligned_load(p) * mul) * mul;
      hash ^= data;
      hash *= mul;
    }
    if ((len & 0x7UL) != 0) {
      const size_t data = load_bytes(end, len & 0x7UL);
      hash ^= data;
      hash *= mul;
    }
    hash = shift_mix(hash) * mul;
    hash = shift_mix(hash);
    return hash;
  }

 private:
  std::size_t seed_;
};

}  // namespace

std::size_t Danger::HashKey(std::string_view key) const noexcept {
  if (!IsRed()) {
    return UnsafeHash(key);
  }

  return SafeHash(key);
}

bool Danger::IsGreen() const noexcept { return state_ == State::kGreen; }
bool Danger::IsYellow() const noexcept { return state_ == State::kYellow; }
bool Danger::IsRed() const noexcept { return state_ == State::kRed; }

void Danger::ToGreen() noexcept {
  UASSERT(state_ == State::kYellow);

  state_ = State::kGreen;
}

void Danger::ToYellow() noexcept {
  UASSERT(state_ == State::kGreen);

  state_ = State::kYellow;
}

void Danger::ToRed() noexcept {
  UASSERT(state_ == State::kYellow);

  state_ = State::kRed;

  do {
    hash_seed_ =
        std::uniform_int_distribution<std::size_t>{}(utils::DefaultRandom());
  } while (hash_seed_ == 0);
}

std::size_t Danger::SafeHash(std::string_view key) const noexcept {
  UASSERT(hash_seed_ != 0);
  UASSERT(IsLowerCase(key));

  // TODO : better hashing
  return utils::StrCaseHash{hash_seed_}(key);
}

std::size_t Danger::UnsafeHash(std::string_view key) noexcept {
  UASSERT(IsLowerCase(key));

  // The seed is chosen in such a way that default request/response headers
  // hash to different values and never collide.
  // TODO : change the value to account finds
  constexpr std::size_t kGoodDefaultSeed = 21;

  // TODO : is MurMur good enough with power of 2 modulo?
  return GccMurMurHasher{kGoodDefaultSeed}(key.data(), key.size());
}

}  // namespace server::http::header_map_impl

USERVER_NAMESPACE_END
