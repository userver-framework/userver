#pragma once

/// @file userver/utils/str_icase_containers.hpp
/// @brief Contains utilities for working with containers with
/// utils::StrCaseHash and utils::StrIcaseHash.
/// @ingroup userver_universal

#include <unordered_map>
#include <unordered_set>

#include <userver/utils/algo.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Converts an unordered [multi-]map or set with the default hash to a
/// map with utils::StrCaseHash. This might be useful when converting
/// guaranteed-to-be-safe data to a common operating format.
template <typename Key, typename Value>
auto WithSafeHash(const std::unordered_map<Key, Value>& map) {
  return utils::AsContainer<std::unordered_map<Key, Value, utils::StrCaseHash>>(
      map);
}

/// @overload
template <typename Key, typename Value>
auto WithSafeHash(std::unordered_map<Key, Value>&& map) {
  return utils::AsContainer<std::unordered_map<Key, Value, utils::StrCaseHash>>(
      std::move(map));
}

/// @overload
template <typename Key, typename Value>
auto WithSafeHash(const std::unordered_multimap<Key, Value>& map) {
  return utils::AsContainer<
      std::unordered_multimap<Key, Value, utils::StrCaseHash>>(map);
}

/// @overload
template <typename Key, typename Value>
auto WithSafeHash(std::unordered_multimap<Key, Value>&& map) {
  return utils::AsContainer<
      std::unordered_multimap<Key, Value, utils::StrCaseHash>>(std::move(map));
}

/// @overload
template <typename Key>
auto WithSafeHash(const std::unordered_set<Key>& map) {
  return utils::AsContainer<std::unordered_set<Key, utils::StrCaseHash>>(map);
}

/// @overload
template <typename Key>
auto WithSafeHash(std::unordered_set<Key>&& map) {
  return utils::AsContainer<std::unordered_set<Key, utils::StrCaseHash>>(
      std::move(map));
}

/// @overload
template <typename Key>
auto WithSafeHash(const std::unordered_multiset<Key>& map) {
  return utils::AsContainer<std::unordered_multiset<Key, utils::StrCaseHash>>(
      map);
}

/// @overload
template <typename Key>
auto WithSafeHash(std::unordered_multiset<Key>&& map) {
  return utils::AsContainer<std::unordered_multiset<Key, utils::StrCaseHash>>(
      std::move(map));
}

}  // namespace utils

USERVER_NAMESPACE_END
