#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#if __cpp_lib_generic_unordered_lookup < 201811L
#error "Your compiler does not support transparent hash lookup"
#endif

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

template <typename Key>
requires std::is_convertible_v<Key, std::string_view>
struct TransparentHash : public std::hash<std::string_view> {
    using is_transparent [[maybe_unused]] = void;
};

template <
    typename Key,
    typename Value,
    typename Hash = TransparentHash<Key>,
    typename Equal = std::equal_to<>,
    typename Allocator = std::allocator<std::pair<const Key, Value>>>
using TransparentMap = std::unordered_map<Key, Value, Hash, Equal, Allocator>;

template <
    typename Key,
    typename Hash = TransparentHash<Key>,
    typename Equal = std::equal_to<>,
    typename Allocator = std::allocator<Key>>
using TransparentSet = std::unordered_set<Key, Hash, Equal, Allocator>;

}  // namespace utils::impl

USERVER_NAMESPACE_END
