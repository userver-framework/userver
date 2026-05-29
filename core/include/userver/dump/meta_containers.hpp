#pragma once

/// @file userver/dump/meta_containers.hpp
/// @brief Dump serialization customization points for STL containers

#include <iterator>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <userver/dump/meta.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace dump {

/// @{
/// Customization point: insert an element into a container
template <typename T, typename Alloc>
void Insert(std::vector<T, Alloc>& cont, T&& elem) {
    cont.push_back(std::forward<T>(elem));
}

template <typename K, typename V, typename Comp, typename Alloc>
void Insert(std::map<K, V, Comp, Alloc>& cont, std::pair<const K, V>&& elem) {
    cont.insert(std::move(elem));
}

template <typename K, typename V, typename Hash, typename Eq, typename Alloc>
void Insert(std::unordered_map<K, V, Hash, Eq, Alloc>& cont, std::pair<const K, V>&& elem) {
    cont.insert(std::move(elem));
}

template <typename T, typename Comp, typename Alloc>
void Insert(std::set<T, Comp, Alloc>& cont, T&& elem) {
    cont.insert(std::forward<T>(elem));
}

template <typename T, typename Hash, typename Eq, typename Alloc>
void Insert(std::unordered_set<T, Hash, Eq, Alloc>& cont, T&& elem) {
    cont.insert(std::forward<T>(elem));
}
/// @}

/// Check if a range is a container
template <typename T>
concept IsContainer =
    meta::kIsRange<T> && std::is_default_constructible_v<T> && meta::kIsSizable<T> &&
    requires(T& t, meta::RangeValueType<T>&& v) { dump::Insert(t, std::move(v)); };

}  // namespace dump

USERVER_NAMESPACE_END
