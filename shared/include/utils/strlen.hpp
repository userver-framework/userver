#pragma once

#include <cstdint>

namespace utils {

/// Constexpr usable strlen
constexpr std::size_t StrLen(const char* str) {
  std::size_t sz{0};
  for (; *str != 0; ++str, ++sz)
    ;
  return sz;
}

}  // namespace utils
