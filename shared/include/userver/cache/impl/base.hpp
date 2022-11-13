#pragma once

#include <functional>
#include <typeindex>
#include "userver/cache/policy.hpp"

USERVER_NAMESPACE_BEGIN

namespace cache::impl {
template <typename T, typename U, typename Hash = std::hash<T>,
          typename Equal = std::equal_to<T>,
          CachePolicy Policy = CachePolicy::kLRU>
class LruBase final {};
}  // namespace cache::impl

USERVER_NAMESPACE_END