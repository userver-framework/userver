#pragma once

#include <cstring>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/header_map.hpp>

#include <server/http/header_map_impl/header_name.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace header_map_impl {

namespace impl {

inline std::size_t unaligned_load(const char* p) {
  std::size_t result{};
  std::memcpy(&result, p, sizeof(result));
  return result;
}

inline constexpr std::size_t load_bytes(const char* p, int n) {
  std::size_t result = 0;
  --n;

  do {
    result = (result << 8) + static_cast<unsigned char>(p[n]);
  } while (--n >= 0);
  return result;
}

inline constexpr std::size_t shift_mix(std::size_t v) { return v ^ (v >> 47); }

// Assert little endianness, otherwise unaligned_load(p) != load_bytes(p, 8) and
// we get different hashes at compile time and at runtime.
static_assert(load_bytes("\1\0\0\0\0\0\0\0", 8) == 1);

// Who even compiles userver for 32 bits.
static_assert(sizeof(std::size_t) == 8);

}  // namespace impl

// MurMur2 implementation taken from
// https://github.com/gcc-mirror/gcc/blob/da3aca031be736fe4fa8daa57c7efa69dc767160/libstdc%2B%2B-v3/libsupc%2B%2B/hash_bytes.cc
template <bool AtCompileTime>
class GccMurMurHasher final {
 public:
  constexpr std::size_t operator()(const char* ptr, std::size_t len) const {
    constexpr std::size_t mul = (0xc6a4a793UL << 32UL) + 0x5bd1e995UL;
    const char* const buf = ptr;

    const std::size_t len_aligned = len & ~0x7UL;
    const char* const end = buf + len_aligned;
    std::size_t hash = seed_ ^ (len * mul);
    for (const char* p = buf; p != end; p += 8) {
      const std::size_t data = [p] {
        if constexpr (AtCompileTime) {
          return impl::shift_mix(impl::load_bytes(p, 8) * mul) * mul;
        } else {
          return impl::shift_mix(impl::unaligned_load(p) * mul) * mul;
        }
      }();
      hash ^= data;
      hash *= mul;
    }
    if ((len & 0x7UL) != 0) {
      const std::size_t data = impl::load_bytes(end, len & 0x7UL);
      hash ^= data;
      hash *= mul;
    }
    hash = impl::shift_mix(hash) * mul;
    hash = impl::shift_mix(hash);
    return hash;
  }

 private:
  // Seed is chosen in such a way that 16 common userver headers all have
  // different remainder modulo 16 and thus don't collide within default
  // map size.
  std::size_t seed_{1114924};
};

using CompileTimeHasher = GccMurMurHasher<true>;
using RuntimeHasher = GccMurMurHasher<false>;

}  // namespace header_map_impl

constexpr SpecialHeader::SpecialHeader(std::string_view name)
    : name{name},
      hash{header_map_impl::CompileTimeHasher{}(name.data(), name.size())} {
  if (!header_map_impl::IsLowerCase(name)) {
    throw std::logic_error{"Special header has to be in lowercase"};
  }
}

namespace http_headers = USERVER_NAMESPACE::http::headers;

inline constexpr SpecialHeader kXYaTraceIdHeader{http_headers::kXYaTraceId};
inline constexpr SpecialHeader kXYaSpanIdHeader{http_headers::kXYaSpanId};
inline constexpr SpecialHeader kXYaRequestIdHeader{http_headers::kXYaRequestId};

inline constexpr SpecialHeader kXRequestIdHeader{http_headers::kXRequestId};
inline constexpr SpecialHeader kXBackendServerHeader{
    http_headers::kXBackendServer};
inline constexpr SpecialHeader kXTaxiEnvoyProxyDstVhostHeader{
    http_headers::kXTaxiEnvoyProxyDstVhost};

inline constexpr SpecialHeader kServerHeader{http_headers::kServer};
inline constexpr SpecialHeader kContentLengthHeader{
    http_headers::kContentLength};
inline constexpr SpecialHeader kDateHeader{http_headers::kDate};
inline constexpr SpecialHeader kContentTypeHeader{http_headers::kContentType};
inline constexpr SpecialHeader kConnectionHeader{http_headers::kConnection};

inline constexpr SpecialHeader kCookieHeader{"cookie"};

inline constexpr SpecialHeader kXYaTaxiClientTimeoutMsHeader{
    http_headers::kXYaTaxiClientTimeoutMs};

}  // namespace server::http

USERVER_NAMESPACE_END
