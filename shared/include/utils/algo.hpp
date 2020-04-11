#pragma once

/// @file utils/algo.hpp
/// @brief Small useful algorithms.

#include <cstddef>

namespace utils {

/// Constexpr usable strlen
constexpr std::size_t StrLen(const char* str) noexcept {
  std::size_t sz{0};
  for (; *str != 0; ++str, ++sz)
    ;
  return sz;
}

/// Returns nullptr if no key in associative container. Otherwise returns
/// pointer to value.
template <class Map, class Key>
auto FindOrNullptr(Map& map, const Key& key) {
  const auto it = map.find(key);
  return (it == map.end() ? nullptr : &it->second);
}

}  // namespace utils
