#include <userver/utils/encoding/hex.hpp>

#include <stdexcept>
#include <string_view>

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
  out.reserve(input.size() * 2);
  auto first = input.data();
  auto last = input.data() + input.size();
  while (first != last) {
    const auto value = *first;
    // We don't use ToHexChar because it does range checking
    out.push_back(detail::kXdigits[(value >> 4) & 0xf]);
    out.push_back(detail::kXdigits[value & 0xf]);
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
