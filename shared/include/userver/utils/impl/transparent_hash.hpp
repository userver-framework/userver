#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#if __cpp_lib_generic_unordered_lookup < 201811L
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#endif

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename Key>
struct TransparentHash : public std::hash<std::string_view> {
  static_assert(std::is_convertible_v<Key, std::string_view>,
                "TransparentHash is only implemented for strings for far");

  using is_transparent [[maybe_unused]] = void;
};

// Use:
// - std::unordered_{map,set} in C++20
// - boost::unordered_{map,set} in C++17

#if __cpp_lib_generic_unordered_lookup >= 201811L
template <typename Key, typename Value, typename Hash = TransparentHash<Key>,
          typename Equal = std::equal_to<>>
using TransparentMap = std::unordered_map<Key, Value, Hash, Equal>;

template <typename Key, typename Hash = TransparentHash<Key>,
          typename Equal = std::equal_to<>>
using TransparentSet = std::unordered_set<Key, Hash, Equal>;
#else
template <typename Key, typename Value, typename Hash = TransparentHash<Key>,
          typename Equal = std::equal_to<>>
using TransparentMap = boost::unordered_map<Key, Value, Hash, Equal>;

template <typename Key, typename Hash = TransparentHash<Key>,
          typename Equal = std::equal_to<>>
using TransparentSet = boost::unordered_set<Key, Hash, Equal>;
#endif

template <typename TransparentContainer, typename Key>
auto FindTransparent(TransparentContainer& container, const Key& key) {
#if __cpp_lib_generic_unordered_lookup >= 201811L
  return container.find(key);
#else
  return container.find(key, container.hash_function(), container.key_eq());
#endif
}

template <typename TransparentMap, typename Key>
auto* FindTransparentOrNullptr(TransparentMap& map, const Key& key) {
  const auto iterator = FindTransparent(map, key);
  return iterator == map.end() ? nullptr : &iterator->second;
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
