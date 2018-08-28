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
  std::string result;
  result.reserve(std::distance(first, last) * 2);
  while (first != last) {
    result.push_back(ToHexChar((*first >> 4) & 0xf));
    result.push_back(ToHexChar(*first & 0xf));
    ++first;
  }
  return result;
}

inline std::string ToHex(const void* data, size_t len) {
  auto* chars = reinterpret_cast<const char*>(data);
  return ToHex(chars, chars + len);
}

}  // namespace encoding
}  // namespace utils
