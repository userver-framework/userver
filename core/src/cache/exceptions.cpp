#include <userver/cache/exceptions.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace cache {

EmptyCacheError::EmptyCacheError(std::string_view cache_name)
    : std::runtime_error(fmt::format("Cache '{}' is empty", cache_name)) {}

EmptyDataError::EmptyDataError(std::string_view cache_name)
    : DataError(
          fmt::format("Overwriting cache '{}' with empty data is not allowed",
                      cache_name)) {}

}  // namespace cache

USERVER_NAMESPACE_END
