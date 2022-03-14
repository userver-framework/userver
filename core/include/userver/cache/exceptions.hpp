#pragma once

/// @file userver/cache/exceptions.hpp
/// @brief Exceptions thrown by components::CachingComponentBase

#include <stdexcept>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// Thrown by components::CachingComponentBase::Get if the cache is empty and
/// `MayReturnNull` returns `false` (as by default).
class EmptyCacheError final : public std::runtime_error {
 public:
  explicit EmptyCacheError(std::string_view cache_name);
};

}  // namespace cache

USERVER_NAMESPACE_END
