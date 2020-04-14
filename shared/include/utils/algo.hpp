#pragma once

/// @file utils/algo.hpp
/// @brief Small useful algorithms.

#include <cstddef>
#include <utility>

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

/// Returns default value if no key in associative container. Otherwise returns
/// pointer to value.
template <class Map, class Key, class Default>
typename Map::mapped_type FindOrDefault(Map& map, const Key& key,
                                        Default&& def) {
  const auto ptr = ::utils::FindOrNullptr(map, key);
  if (!ptr) {
    return {std::forward<Default>(def)};
  }
  return *ptr;
}

}  // namespace utils
