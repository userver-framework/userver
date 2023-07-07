#pragma once

#include <string>
#include <string_view>

#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::headers {

namespace impl {

// This is a constexpr implementation of case-insensitive MurMur hash for 64-bit
// size_t and Little Endianness.
// https://github.com/gcc-mirror/gcc/blob/65369ab62cee68eb7f6ef65e3d12d1969a9e20ee/libstdc%2B%2B-v3/libsupc%2B%2B/hash_bytes.cc#L138
//
// P.S. The hasher is "unsafe" in hash-flood sense.
struct UnsafeConstexprHasher final {
  constexpr std::size_t operator()(std::string_view str) const noexcept {
    constexpr std::uint64_t mul = (0xc6a4a793UL << 32UL) + 0x5bd1e995UL;

    std::uint64_t hash = seed_ ^ (str.size() * mul);
    while (str.size() >= 8) {
      const std::uint64_t data = ShiftMix(Load8(str.data()) * mul) * mul;
      hash ^= data;
      hash *= mul;

      str = str.substr(8);
    }
    if (!str.empty()) {
      const std::uint64_t data = LoadN(str.data(), str.size());
      hash ^= data;
      hash *= mul;
    }

    hash = ShiftMix(hash) * mul;
    hash = ShiftMix(hash);
    return hash;
  }

 private:
  static constexpr inline std::uint64_t ShiftMix(std::uint64_t v) noexcept {
    return v ^ (v >> 47);
  }

  static constexpr inline std::uint64_t Load8(const char* data) noexcept {
    return LoadN(data, 8);
  }

  static constexpr inline std::uint64_t LoadN(const char* data,
                                              std::size_t n) noexcept {
    // Although lowercase and uppercase ASCII are indeed 32 (0x20) apart,
    // this approach makes for instance '[' and '{' equivalent too,
    // which is obviously broken and is easily exploitable.
    // However, for expected input (lower/upper-case ASCII letters + dashes)
    // this just works, and against malicious
    // input we defend by falling back to case-insensitive SipHash.
    constexpr std::uint64_t kDeliberatelyBrokenLowercaseMask =
        0x2020202020202020UL;

    std::uint64_t result = kDeliberatelyBrokenLowercaseMask >> (8 * (8 - n));
    for (std::size_t i = 0; i < n; ++i) {
      const std::uint8_t c = data[i];
      result |= static_cast<std::uint64_t>(c) << (8 * i);
    }
    return result;
  }

  // Seed is chosen in such a way that 16 (presumably) most common userver
  // headers don't collide within default size of HeaderMap (32),
  // and that all headers used in userver itself don't collide within minimal
  // size needed to store them all (36 headers, minimal size = 64).
  // Note that since HeaderMap takes hashes modulo power of 2 it's guaranteed
  // that if two headers don't collide within size S, they don't collide for
  // bigger sizes.
  std::uint64_t seed_{54999};
};

// Ugly, but TrivialBiMap requires keys to be in lower case.
inline constexpr utils::TrivialBiMap kKnownHeadersLowercaseMap =
    [](auto selector) {
      return selector()
          .Case("content-type", 1)
          .Case("content-encoding", 2)
          .Case("content-length", 3)
          .Case("transfer-encoding", 4)
          .Case("host", 5)
          .Case("accept", 6)
          .Case("accept-encoding", 7)
          .Case("accept-language", 8)
          .Case("x-yataxi-api-key", 9)
          .Case("user-agent", 10)
          .Case("x-request-application", 11)
          .Case("date", 12)
          .Case("warning", 13)
          .Case("access-control-allow-headers", 14)
          .Case("allow", 15)
          .Case("server", 16)
          .Case("set-cookie", 17)
          .Case("connection", 18)
          .Case("cookie", 19)
          .Case("x-yarequestid", 20)
          .Case("x-yatraceid", 21)
          .Case("x-yaspanid", 22)
          .Case("x-requestid", 23)
          .Case("x-backend-server", 24)
          .Case("x-taxi-envoyproxy-dstvhost", 25)
          .Case("baggage", 26)
          .Case("x-yataxi-allow-auth-request", 27)
          .Case("x-yataxi-allow-auth-response", 28)
          .Case("x-yataxi-server-hostname", 29)
          .Case("x-yataxi-client-timeoutms", 30)
          .Case("x-yataxi-ratelimited-by", 31)
          .Case("x-yataxi-ratelimit-reason", 32);
    };

// We use different values for "no index" at compile and run time to simplify
// comparison - with these values being different we cant just == them.
constexpr std::int8_t kNoHeaderIndexLookup = -1;
constexpr std::int8_t kNoHeaderIndexInsertion = -2;
static_assert(kNoHeaderIndexLookup != kNoHeaderIndexInsertion);
static_assert(kNoHeaderIndexLookup != 0 && kNoHeaderIndexInsertion != 0);

// We use this function when constructing a PredefinedHeader ...
constexpr std::int8_t GetHeaderIndexForLookup(std::string_view key) {
  return kKnownHeadersLowercaseMap.TryFindICaseByFirst(key).value_or(
      kNoHeaderIndexLookup);
}

// And this one when inserting an entry into the HeaderMap.
// The purpose of having 2 different functions is to be able to
// == header indexes even if none is present (both headers are unknown).
inline std::int8_t GetHeaderIndexForInsertion(std::string_view key) {
  return kKnownHeadersLowercaseMap.TryFindICaseByFirst(key).value_or(
      kNoHeaderIndexInsertion);
}

}  // namespace impl

namespace header_map {
class Danger;
class Map;
}  // namespace header_map

/// @brief A struct to represent compile-time known header name.
///
/// Calculates the hash value at compile time with the same hasher
/// HeaderMap uses, which allows to speed things up greatly.
///
/// Although it's possible to construct PredefinedHeader at runtime
/// it makes little sense and is error-prone, since it
/// doesn't own its data, so don't do that until really needed.
class PredefinedHeader final {
 public:
  explicit constexpr PredefinedHeader(std::string_view name)
      : name{name},
        hash{impl::UnsafeConstexprHasher{}(name)},
        header_index{impl::GetHeaderIndexForLookup(name)} {}

  constexpr operator std::string_view() const { return name; }

  explicit operator std::string() const { return std::string{name}; }

 private:
  friend class header_map::Danger;
  friend class header_map::Map;

  // Header name.
  const std::string_view name;

  // Unsafe constexpr hash (unsafe in a hash-flood sense).
  const std::size_t hash;

  // We assign a different 'index' value to every known header,
  // which allows us to do not perform case-insensitive names compare if indexes
  // match.
  // With this trick a successful lookup for PredefinedHeader in HeaderMap
  // is basically "access an array by index and compare both hash and index".
  // You can think of this field as an enum discriminant.
  const std::int8_t header_index;
};

}  // namespace http::headers

USERVER_NAMESPACE_END
