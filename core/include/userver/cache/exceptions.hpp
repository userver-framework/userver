#pragma once

/// @file userver/cache/exceptions.hpp
/// @brief Exceptions thrown by components::CachingComponentBase

#include <stdexcept>
#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace cache {

/// Thrown by components::CachingComponentBase::Get if the cache is empty and
/// `MayReturnNull` returns `false` (as by default).
class EmptyCacheError final : public std::runtime_error {
 public:
  explicit EmptyCacheError(std::string_view cache_name);
};

/// Thrown by components::CachingComponentBase::PreAssignCheck when the new
/// value does not pass the check.
class DataError : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

class EmptyDataError final : public DataError {
 public:
  explicit EmptyDataError(std::string_view cache_name);
};

}  // namespace cache

USERVER_NAMESPACE_END
