#pragma once

/// @file utils/algo.hpp
/// @brief Small useful algorithms.

#include <cstddef>
#include <optional>
#include <utility>

#include <utils/checked_pointer.hpp>

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

/// Returns std::nullopt if no key in associative container. Otherwise returns
/// std::optional with a copy of value
template <class Map, class Key>
std::optional<typename Map::mapped_type> FindOptional(Map& map,
                                                      const Key& key) {
  const auto ptr = ::utils::FindOrNullptr(map, key);
  if (!ptr) {
    return std::nullopt;
  }
  return {*ptr};
}

/// Search a map for an element and return a checked pointer to the found
/// element
template <typename Map, typename Key>
auto CheckedFind(Map& map, const Key& key)
    -> decltype(MakeCheckedPtr(&map.find(key)->second)) {
  if (auto f = map.find(key); f != map.end()) {
    return MakeCheckedPtr(&f->second);
  }
  return nullptr;
}

}  // namespace utils
