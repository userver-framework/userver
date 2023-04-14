#include <utils/impl/byte_utils.hpp>

#ifdef __SSE2__
#include <immintrin.h>
#endif

#include <array>
#include <cstring>
#include <limits>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

namespace {

inline std::uint64_t RotateLeft(std::uint64_t x, std::uint64_t b) noexcept {
  return (x << b) | (x >> (64UL - b));
}

struct CaseFetcher final {
  static inline std::uint64_t Fetch8(const std::uint8_t* data) noexcept {
    std::uint64_t result{};
    std::memcpy(&result, data, 8);
    return result;
  }

  static inline std::pair<std::uint64_t, std::uint64_t> Fetch16(
      const std::uint8_t* data) noexcept {
    return {Fetch8(data), Fetch8(data + 8)};
  }

  static inline std::uint64_t FetchN(const std::uint8_t* data,
                                     std::size_t n) noexcept {
    UASSERT(n < 8);

    std::uint64_t res = 0;
    // n is always <= 7 by algorithm construction
    // NOLINTNEXTLINE(hicpp-multiway-paths-covered)
    switch (n) {
      case 7:
        res |= static_cast<std::uint64_t>(data[6]) << 48;
        [[fallthrough]];
      case 6:
        res |= static_cast<std::uint64_t>(data[5]) << 40;
        [[fallthrough]];
      case 5:
        res |= static_cast<std::uint64_t>(data[4]) << 32;
        [[fallthrough]];
      case 4:
        res |= static_cast<std::uint64_t>(data[3]) << 24;
        [[fallthrough]];
      case 3:
        res |= static_cast<std::uint64_t>(data[2]) << 16;
        [[fallthrough]];
      case 2:
        res |= static_cast<std::uint64_t>(data[1]) << 8;
        [[fallthrough]];
      case 1: {
        res |= static_cast<std::uint64_t>(data[0]);
        break;
      }
      case 0:
        break;
    }

    return res;
  }
};

#ifdef __SSE2__
struct CaseInsensitiveSSEFetcher final {
  static inline std::uint64_t Fetch8(const uint8_t* data) noexcept {
    // _mm_loadu_si64 is missing in gcc prior to version 9
    // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=78782
#if defined(__GNUC__) && !defined(__llvm__) && (__GNUC__ < 9)
    const auto value = CaseFetcher::Fetch8(data);
    return LowercaseBytes(
               _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&value)))
        .first;
#else
    return LowercaseBytes(_mm_loadu_si64(data)).first;
#endif
  }

  static inline std::pair<std::uint64_t, std::uint64_t> Fetch16(
      const uint8_t* data) noexcept {
    return LowercaseBytes(
        _mm_loadu_si128(reinterpret_cast<const __m128i*>(data)));
  }

  static inline std::uint64_t FetchN(const uint8_t* data,
                                     std::size_t n) noexcept {
    const auto value = CaseFetcher::FetchN(data, n);
    return LowercaseBytes(
               _mm_loadl_epi64(reinterpret_cast<const __m128i*>(&value)))
        .first;
  }

 private:
  static inline std::pair<std::uint64_t, std::uint64_t> LowercaseBytes(
      __m128i value) noexcept {
    const auto kMaskA = _mm_set1_epi8('A');
    const auto kMaskZ = _mm_set1_epi8('Z');
    const auto kMask32 = _mm_set1_epi8(32);

    // mask_az contains 0x00 where character is between 'A' and 'Z',
    // 0xff otherwise
    const auto mask_az = _mm_or_si128(_mm_cmplt_epi8(value, kMaskA),
                                      _mm_cmpgt_epi8(value, kMaskZ));
    // although _mm_cmp[lt|gt]_epi8 compares SIGNED 8-bits values,
    // which seems dangerous, it's fine because for the resulting value
    // to be 0x00 (what we treat as an uppercase character) the original value
    // has to be in ['A'; 'Z']: the construction above is actually just a
    // negation of this condition (and that's why we are interested in 0x00
    // rather than in 0xff):
    // c >= 'A' && c <= 'Z' <=> !('c' < 'A' || c > 'Z').
    // Since no negative value falls in this range we are good.
    static_assert(static_cast<int>('Z') <=
                  std::numeric_limits<std::int8_t>::max());

    // set 6-th bit to 1 for uppercase characters
    const auto lowercase_mask = _mm_andnot_si128(mask_az, kMask32);
    const auto lowercase = _mm_or_si128(value, lowercase_mask);

    std::array<std::uint64_t, 2> result{};
    std::memcpy(result.data(), &lowercase, 16);
    return {result[0], result[1]};
  }
};
#endif

struct CaseInsensitiveFetcher final {
  static inline std::uint64_t Fetch8(const std::uint8_t* data) noexcept {
    std::array<std::uint8_t, 8> lowercase{};
    Lowercase(lowercase.data(), data, 8);
    return CaseFetcher::Fetch8(lowercase.data());
  }

  static inline std::pair<std::uint64_t, std::uint64_t> Fetch16(
      const std::uint8_t* data) noexcept {
    std::array<std::uint8_t, 16> lowercase{};
    Lowercase(lowercase.data(), data, 16);
    return CaseFetcher::Fetch16(lowercase.data());
  }

  static inline std::uint64_t FetchN(const std::uint8_t* data,
                                     std::size_t n) noexcept {
    // n should be less than 8 by algorithm construction
    UASSERT(n < 8);

    std::array<std::uint8_t, 8> lowercase{};
    Lowercase(lowercase.data(), data, n);
    return CaseFetcher::FetchN(lowercase.data(), n);
  }

 private:
  static inline void Lowercase(std::uint8_t* dst, const std::uint8_t* src,
                               std::size_t len) noexcept {
    for (std::size_t i = 0; i < len; ++i) {
      const char c = src[i];
      const char mask = (c >= 'A' && c <= 'Z') ? 32 : 0;
      dst[i] = c | mask;
    }
  }
};

template <typename Fetcher>
// We hard-code 1-3 rounds, because that's enough for our use case.
// Should be easy to parametrize, if ever needed.
std::uint64_t SipHash13(std::uint64_t k0, std::uint64_t k1,
                        std::string_view data) noexcept {
  std::uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
  std::uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
  std::uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
  std::uint64_t v3 = k1 ^ 0x7465646279746573ULL;

  uint64_t b = data.size() << 56;

  const auto sip_round = [&v0, &v1, &v2, &v3] {
    v0 += v1;
    v1 = RotateLeft(v1, 13);
    v1 ^= v0;
    v0 = RotateLeft(v0, 32);
    v2 += v3;
    v3 = RotateLeft(v3, 16);
    v3 ^= v2;
    v0 += v3;
    v3 = RotateLeft(v3, 21);
    v3 ^= v0;
    v2 += v1;
    v1 = RotateLeft(v1, 17);
    v1 ^= v2;
    v2 = RotateLeft(v2, 32);
  };

  const auto data_round = [&v3, &v0, &sip_round](std::uint64_t m) {
    v3 ^= m;
    sip_round();
    v0 ^= m;
  };

  // This is just an unrolled version of while (data.size() >= 8).
  // We use SSE2 to lower-case data, and lower-casing 8 bytes takes exactly the
  // same amount of instructions as lower-casing 16 bytes, so why not.
  while (data.size() >= 16) {
    const auto [m1, m2] =
        Fetcher::Fetch16(reinterpret_cast<const std::uint8_t*>(data.data()));
    data_round(m1);
    data_round(m2);

    data = data.substr(16);
  }

  if (data.size() >= 8) {
    const std::uint64_t m =
        Fetcher::Fetch8(reinterpret_cast<const std::uint8_t*>(data.data()));
    data_round(m);

    data = data.substr(8);
  }

  b |= Fetcher::FetchN(reinterpret_cast<const std::uint8_t*>(data.data()),
                       data.size());
  data_round(b);

  v2 ^= 0xff;
  sip_round();
  sip_round();
  sip_round();

  return v0 ^ v1 ^ v2 ^ v3;
}

}  // namespace

SipHasher::SipHasher(std::uint64_t k0, std::uint64_t k1) noexcept
    : k0_{k0}, k1_{k1} {}

std::uint64_t SipHasher::operator()(std::string_view data) const noexcept {
  return SipHash13<CaseFetcher>(k0_, k1_, data);
}

CaseInsensitiveSipHasher::CaseInsensitiveSipHasher(std::uint64_t k0,
                                                   std::uint64_t k1) noexcept
    : k0_{k0}, k1_{k1} {}

std::uint64_t CaseInsensitiveSipHasher::operator()(std::string_view data) const
    noexcept {
#ifdef __SSE2__
  return SipHash13<CaseInsensitiveSSEFetcher>(k0_, k1_, data);
#else
  return CaseInsensitiveSipHasherNoSse{k0_, k1_}(data);
#endif
}

CaseInsensitiveSipHasherNoSse::CaseInsensitiveSipHasherNoSse(
    std::uint64_t k0, std::uint64_t k1) noexcept
    : k0_{k0}, k1_{k1} {}

std::uint64_t CaseInsensitiveSipHasherNoSse::operator()(
    std::string_view data) const noexcept {
  return SipHash13<CaseInsensitiveFetcher>(k0_, k1_, data);
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
