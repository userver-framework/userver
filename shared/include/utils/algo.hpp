#pragma once

/// @file utils/algo.hpp
/// @brief Small useful algorithms.

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
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

template <typename... Strings>
std::string StrCat(Strings&&... strings) {
  std::size_t result_size = 0;
  ((result_size += std::string_view{strings}.size()), ...);

  std::string result;
  result.reserve(result_size);
  (result.append(strings), ...);
  return result;
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

/// Convert one container type to another
template <class ToContainer, class FromContainer>
ToContainer AsContainer(FromContainer&& container) {
  if constexpr (std::is_rvalue_reference_v<decltype(container)>) {
    return ToContainer(std::make_move_iterator(std::begin(container)),
                       std::make_move_iterator(std::end(container)));
  } else {
    return ToContainer(std::begin(container), std::end(container));
  }
}

namespace impl {
template <typename Container, typename = void>
struct HasKeyType : std::false_type {};

template <typename Container>
struct HasKeyType<Container, std::void_t<typename Container::key_type>>
    : std::true_type {};

template <typename Container>
inline constexpr bool kHasKeyType = HasKeyType<Container>::value;
}  // namespace impl

/// @returns number of deleted elements
template <class Container, class Pred>
auto EraseIf(Container& container, Pred pred) {
  if constexpr (impl::kHasKeyType<Container>) {
    auto old_size = container.size();
    for (auto it = std::begin(container), last = std::end(container);
         it != last;) {
      if (pred(*it)) {
        it = container.erase(it);
      } else {
        ++it;
      }
    }
    return old_size - container.size();
  } else {
    auto it = std::remove_if(std::begin(container), std::end(container), pred);
    const auto removed = std::distance(it, std::end(container));
    container.erase(it, std::end(container));
    return removed;
  }
}

/// @returns number of deleted elements
template <class Container, class T>
size_t Erase(Container& container, const T& elem) {
  if constexpr (impl::kHasKeyType<Container>) {
    return container.erase(elem);
  } else {
    auto it = std::remove(std::begin(container), std::end(container), elem);
    const auto removed = std::distance(it, std::end(container));
    container.erase(it, std::end(container));
    return removed;
  }
}

/// @returns if there is an element in container which satisfies predicate
template <typename Container, typename Pred>
bool ContainsIf(const Container& container, Pred pred) {
  return std::find_if(std::begin(container), std::end(container), pred) !=
         std::end(container);
}

}  // namespace utils
