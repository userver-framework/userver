#include <userver/utils/encoding/hex.hpp>

#include <stdexcept>
#include <string_view>

#ifdef __SSSE3__
#include <tmmintrin.h>
#endif

USERVER_NAMESPACE_BEGIN

namespace utils::encoding {

namespace detail {

static constexpr std::string_view kXdigits = "0123456789abcdef";
static_assert(kXdigits.size() == 16);

char ToHexChar(int num) {
  if (num >= 16 || num < 0) {
    throw std::runtime_error(std::to_string(num) +
                             " is not convertible to hex char");
  }
  return detail::kXdigits[num];
}

/// Converts xDigit to its value. Undefined behaviour if
/// xDigit is not one of "0123456789abcdef"
unsigned char GetXDigitValue(unsigned char x_digit) noexcept {
  switch (x_digit) {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    case 'a':
    case 'A':
      return 10;
    case 'b':
    case 'B':
      return 11;
    case 'c':
    case 'C':
      return 12;
    case 'd':
    case 'D':
      return 13;
    case 'e':
    case 'E':
      return 14;
    case 'f':
    case 'F':
      return 15;
    default:
      return 255;
  }

  return 255;  // because undefined behaviour
}

bool IsXDigit(unsigned char x_digit) noexcept {
  switch (x_digit) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case 'a':
    case 'A':
    case 'b':
    case 'B':
    case 'c':
    case 'C':
    case 'd':
    case 'D':
    case 'e':
    case 'E':
    case 'f':
    case 'F':
      return true;
    default:
      return false;
  }

  return false;
}

#ifdef __SSSE3__
const auto kLow4BitsMask = _mm_set1_epi8(0xf);
const auto kDigitsMask = _mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7',
                                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f');
#endif

}  // namespace detail

std::string_view GetHexPart(std::string_view encoded) noexcept {
  const char* ptr = encoded.data();
  const char* last = ptr + encoded.size();
  for (; ptr != last; ptr++) {
    if (!detail::IsXDigit(*ptr)) {
      break;
    }
  }

  auto length = std::distance(encoded.data(), ptr);
  if (length % 2 == 1) {
    // uneven length
    length--;
  }

  return std::string_view{encoded.data(), static_cast<size_t>(length)};
}

void ToHex(std::string_view input, std::string& out) noexcept {
  out.clear();
  out.resize(input.size() * 2);
  const auto* first = input.data();
  const auto* last = input.data() + input.size();
  auto* dst = out.data();

#ifdef __SSSE3__
  while (last - first >= 8) {
    // we only take 8 bytes because each byte transforms into 2 bytes
    // (first digit comes from 4 high bits, second comes from 4 low bits)
    const auto eight_bytes_of_data = _mm_loadu_si64(first);

    // we take the original eight bytes, shift them (as one 64-bits integer)
    // 4 bits to the right - now we have 4 high bits of each original byte
    // in the lowest 4 bits, with some garbage in higher bits, - combine
    // the original 8 bytes interleaved with it and mask out
    // highest 4 bits of each byte. So we get this in the end:
    // h4(b0), l4(b0), h4(b1), l4(b1), ... where h4() is the highest 4 bits,
    // l4() - lowest 4 bits, and b0, b1, ... are the original bytes
    const auto interleaving_hi_lo =
        _mm_and_si128(_mm_unpacklo_epi8(_mm_srli_epi64(eight_bytes_of_data, 4),
                                        eight_bytes_of_data),
                      detail::kLow4BitsMask);

    // and now we gather kXdigits as specified in interleaving_hi_lo
    // and store them into the result
    _mm_storeu_si128(reinterpret_cast<__m128i*>(dst),
                     _mm_shuffle_epi8(detail::kDigitsMask, interleaving_hi_lo));

    first += 8;
    dst += 16;
  }
#endif

  while (first != last) {
    const auto value = *first;
    // We don't use ToHexChar because it does range checking
    *(dst++) = detail::kXdigits[(value >> 4) & 0xf];
    *(dst++) = detail::kXdigits[value & 0xf];
    ++first;
  }
}

bool IsHexData(std::string_view encoded) noexcept {
  // Checking length is the fasted possible way to quickly discard
  // odd-sized input. But, only if iterator is random access
  if (encoded.size() % 2 != 0) {
    return false;
  }

  return GetHexPart(encoded).size() == encoded.size();
}

size_t FromHex(std::string_view encoded, std::string& out) noexcept {
  // we need to read in pairs
  const char* first = encoded.data();
  const char* pair_ptr = first;
  const char* last = first + encoded.size();
  for (; pair_ptr != last; pair_ptr += 2) {
    if (!detail::IsXDigit(pair_ptr[0])) {
      break;
    }

    if (pair_ptr + 1 == last) {
      // no second element
      break;
    }
    if (!detail::IsXDigit(pair_ptr[1])) {
      break;
    }

    out.push_back((detail::GetXDigitValue(pair_ptr[0]) << 4) |
                  (detail::GetXDigitValue(pair_ptr[1])));
  }

  return static_cast<size_t>(std::distance(first, pair_ptr));
}

}  // namespace utils::encoding

USERVER_NAMESPACE_END
