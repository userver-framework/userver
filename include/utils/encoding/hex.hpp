#pragma once

#include <string>

namespace utils {
namespace encoding {

inline char ToHexChar(int num) {
  static const std::string kXdigits{"0123456789abcdef"};
  return kXdigits.at(num);
}

template <typename It>
std::string ToHex(It first, It last) {
  static_assert(sizeof(*first) == 1, "can only convert chars");
  std::string result(std::distance(first, last) * 2, 'x');
  auto it = result.begin();
  while (first != last) {
    const auto value = *first;
    *it++ = ToHexChar((value >> 4) & 0xf);
    *it++ = ToHexChar(value & 0xf);
    ++first;
  }
  return result;
}

inline std::string ToHex(const void* data, size_t len) {
  auto* chars = reinterpret_cast<const char*>(data);
  return ToHex(chars, chars + len);
}

inline std::string ToHexString(uint64_t value) {
  return ToHex(&value, sizeof(value));
}

}  // namespace encoding
}  // namespace utils
