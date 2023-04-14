#include <gtest/gtest.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <cstring>

#include <utils/impl/byte_utils.hpp>

USERVER_NAMESPACE_BEGIN
namespace {

constexpr std::array<std::uint64_t, 2> seed{9621534751069176051UL,
                                            2054564862222048242UL};
}

namespace reference_siphash_implementation {

// clang-format off
// NOLINTBEGIN

// The code below was takes from https://github.com/antirez/siphash/blob/75c6c6c3d99ba1824c4e93b7852c3f0b2e22134a/siphash.c
// with slight modifications:
// 1. 2-4 to 1-3 (because that's what we use),
// 2. [[fallthrough]]; where needed to fix warnings with Wimplicit-fallthrough

/*
   SipHash reference C implementation

   Copyright (c) 2012-2016 Jean-Philippe Aumasson
   <jeanphilippe.aumasson@gmail.com>
   Copyright (c) 2012-2014 Daniel J. Bernstein <djb@cr.yp.to>
   Copyright (c) 2017 Salvatore Sanfilippo <antirez@gmail.com>
   Copyright (c) 2023 Yandex LLC

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along
   with this software. If not, see
   <http://creativecommons.org/publicdomain/zero/1.0/>.

   ----------------------------------------------------------------------------

   This version was modified by Salvatore Sanfilippo <antirez@gmail.com>
   in the following ways:

   1. Hard-code 2-4 rounds in the hope the compiler can optimize it more
      in this raw from. Anyway we always want the standard 2-4 variant.
   2. Modify the prototype and implementation so that the function directly
      returns an uint64_t value, the hash itself, instead of receiving an
      output buffer. This also means that the output size is set to 8 bytes
      and the 16 bytes output code handling was removed.
   3. Provide a case insensitive variant to be used when hashing strings that
      must be considered identical by the hash table regardless of the case.
      If we don't have directly a case insensitive hash function, we need to
      perform a text transformation in some temporary buffer, which is costly.
   4. Remove debugging code.
   5. Modified the original test.c file to be a stand-alone function testing
      the function in the new form (returing an uint64_t) using just the
      relevant test vector.

   ----------------------------------------------------------------------------

   This version was modified by Yandex LLC in the following ways:

   1. Hard-code 1-3 rounds instead of 2-4.
   2. Insert [[fallthrough]] to fix warnings with Wimplicit-fallthrough.

 */
#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v)                                                        \
    (p)[0] = (uint8_t)((v));                                                   \
    (p)[1] = (uint8_t)((v) >> 8);                                              \
    (p)[2] = (uint8_t)((v) >> 16);                                             \
    (p)[3] = (uint8_t)((v) >> 24);

#define U64TO8_LE(p, v)                                                        \
    U32TO8_LE((p), (uint32_t)((v)));                                           \
    U32TO8_LE((p) + 4, (uint32_t)((v) >> 32));

#define U8TO64_LE(p)                                                           \
    (((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) |                        \
     ((uint64_t)((p)[2]) << 16) | ((uint64_t)((p)[3]) << 24) |                 \
     ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) |                 \
     ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))

#define U8TO64_LE_NOCASE(p)                                                    \
    (((uint64_t)(tolower((p)[0]))) |                                           \
     ((uint64_t)(tolower((p)[1])) << 8) |                                      \
     ((uint64_t)(tolower((p)[2])) << 16) |                                     \
     ((uint64_t)(tolower((p)[3])) << 24) |                                     \
     ((uint64_t)(tolower((p)[4])) << 32) |                                     \
     ((uint64_t)(tolower((p)[5])) << 40) |                                     \
     ((uint64_t)(tolower((p)[6])) << 48) |                                     \
     ((uint64_t)(tolower((p)[7])) << 56))

#define SIPROUND                                                               \
    do {                                                                       \
        v0 += v1;                                                              \
        v1 = ROTL(v1, 13);                                                     \
        v1 ^= v0;                                                              \
        v0 = ROTL(v0, 32);                                                     \
        v2 += v3;                                                              \
        v3 = ROTL(v3, 16);                                                     \
        v3 ^= v2;                                                              \
        v0 += v3;                                                              \
        v3 = ROTL(v3, 21);                                                     \
        v3 ^= v0;                                                              \
        v2 += v1;                                                              \
        v1 = ROTL(v1, 17);                                                     \
        v1 ^= v2;                                                              \
        v2 = ROTL(v2, 32);                                                     \
    } while (0)

uint64_t siphash(const uint8_t *in, const size_t inlen, const uint8_t *k) {
    uint64_t hash;
    uint8_t *out = (uint8_t*) &hash;
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t k0 = U8TO64_LE(k);
    uint64_t k1 = U8TO64_LE(k + 8);
    uint64_t m;
    const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; in != end; in += 8) {
        m = U8TO64_LE(in);
        v3 ^= m;

        SIPROUND;

        v0 ^= m;
    }

    switch (left) {
    case 7: b |= ((uint64_t)in[6]) << 48;
    [[fallthrough]]; case 6: b |= ((uint64_t)in[5]) << 40;
    [[fallthrough]]; case 5: b |= ((uint64_t)in[4]) << 32;
    [[fallthrough]]; case 4: b |= ((uint64_t)in[3]) << 24;
    [[fallthrough]]; case 3: b |= ((uint64_t)in[2]) << 16;
    [[fallthrough]]; case 2: b |= ((uint64_t)in[1]) << 8;
    [[fallthrough]]; case 1: b |= ((uint64_t)in[0]); break;
    case 0: break;
    }

    v3 ^= b;

    SIPROUND;

    v0 ^= b;
    v2 ^= 0xff;

    SIPROUND;
    SIPROUND;
    SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out, b);

    return hash;
}

uint64_t siphash_nocase(const uint8_t *in, const size_t inlen, const uint8_t *k)
{
    uint64_t hash;
    uint8_t *out = (uint8_t*) &hash;
    uint64_t v0 = 0x736f6d6570736575ULL;
    uint64_t v1 = 0x646f72616e646f6dULL;
    uint64_t v2 = 0x6c7967656e657261ULL;
    uint64_t v3 = 0x7465646279746573ULL;
    uint64_t k0 = U8TO64_LE(k);
    uint64_t k1 = U8TO64_LE(k + 8);
    uint64_t m;
    const uint8_t *end = in + inlen - (inlen % sizeof(uint64_t));
    const int left = inlen & 7;
    uint64_t b = ((uint64_t)inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; in != end; in += 8) {
        m = U8TO64_LE_NOCASE(in);
        v3 ^= m;

        SIPROUND;

        v0 ^= m;
    }

    switch (left) {
    case 7: b |= ((uint64_t)tolower(in[6])) << 48;
    [[fallthrough]]; case 6: b |= ((uint64_t)tolower(in[5])) << 40;
    [[fallthrough]]; case 5: b |= ((uint64_t)tolower(in[4])) << 32;
    [[fallthrough]]; case 4: b |= ((uint64_t)tolower(in[3])) << 24;
    [[fallthrough]]; case 3: b |= ((uint64_t)tolower(in[2])) << 16;
    [[fallthrough]]; case 2: b |= ((uint64_t)tolower(in[1])) << 8;
    [[fallthrough]]; case 1: b |= ((uint64_t)tolower(in[0])); break;
    case 0: break;
    }

    v3 ^= b;

    SIPROUND;

    v0 ^= b;
    v2 ^= 0xff;

    SIPROUND;
    SIPROUND;
    SIPROUND;

    b = v0 ^ v1 ^ v2 ^ v3;
    U64TO8_LE(out, b);

    return hash;
}
// CC0 Public Domain Dedication of SipHash ends here, code below is licensed
// under default userver license.

// NOLINTEND
// clang-format on

std::uint64_t Hash(std::string_view data) {
  return reference_siphash_implementation::siphash(
      reinterpret_cast<const std::uint8_t*>(data.data()), data.size(),
      reinterpret_cast<const std::uint8_t*>(seed.data()));
}

std::uint64_t CaseInsensitiveHash(std::string_view data) {
  return reference_siphash_implementation::siphash_nocase(
      reinterpret_cast<const std::uint8_t*>(data.data()), data.size(),
      reinterpret_cast<const std::uint8_t*>(seed.data()));
}

}  // namespace reference_siphash_implementation

namespace {

const std::string kAllPossibleBytesString = [] {
  // 256 for all possible values,
  // +7 to stress the "gather leftovers" step.
  constexpr std::size_t len = 256 + 7;
  std::string result(len, 0);
  for (std::size_t i = 0; i < len; ++i) {
    result[i] = static_cast<char>(i & 0xff);
  }
  return result;
}();

struct ReferenceCaseHash final {
  std::uint64_t operator()(std::string_view data) const noexcept {
    return reference_siphash_implementation::Hash(data);
  }
};

struct ReferenceNoCaseHash final {
  std::uint64_t operator()(std::string_view data) const noexcept {
    return reference_siphash_implementation::CaseInsensitiveHash(data);
  }
};

template <typename Hasher, typename ReferenceHash>
void TestSipHashAgainstReference() {
  const std::string some_random_string{"AsddDDzKJijjwker!233'DD0-MLzxjiho"};

  const Hasher hasher{seed[0], seed[1]};
  const ReferenceHash reference_hasher{};

  {
    // test some random string
    EXPECT_EQ(hasher(some_random_string), reference_hasher(some_random_string));
  }

  {
    // test all possible chars
    for (std::size_t i = 0; i < kAllPossibleBytesString.size(); ++i) {
      const auto data_view =
          std::string_view{kAllPossibleBytesString}.substr(0, i);
      ASSERT_EQ(hasher(data_view), reference_hasher(data_view));
    }
  }

  {
    // test all possible chars at all possible positions
    const auto all_possible_bytes_twice =
        kAllPossibleBytesString + kAllPossibleBytesString;

    for (std::size_t i = 0; i < kAllPossibleBytesString.size(); ++i) {
      const auto data_view = std::string_view{all_possible_bytes_twice}.substr(
          i, kAllPossibleBytesString.size());
      ASSERT_EQ(hasher(data_view), reference_hasher(data_view));
    }
  }
}

}  // namespace

TEST(SipHashCase, MatchesReferenceImplementation) {
  TestSipHashAgainstReference<utils::impl::SipHasher, ReferenceCaseHash>();
}

TEST(SipHashCaseInsensitive, MatchesReferenceImplementation) {
  TestSipHashAgainstReference<utils::impl::CaseInsensitiveSipHasher,
                              ReferenceNoCaseHash>();
}

TEST(SipHashCaseInsensitiveNoSse, MatchesReferenceImplementation) {
  TestSipHashAgainstReference<utils::impl::CaseInsensitiveSipHasherNoSse,
                              ReferenceNoCaseHash>();
}

USERVER_NAMESPACE_END
