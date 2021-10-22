#pragma once

#include <string_view>

/// Algorithms to aid logging
USERVER_NAMESPACE_BEGIN

namespace utils::log {

/// @brief returns `data` converted to hex and truncated to `limit`
std::string ToLimitedHex(std::string_view data, size_t limit);

/// @brief if `data` in utf-8, returns `data` truncated to `limit`
/// otherwise returns stub
std::string ToLimitedUtf8(std::string_view data, size_t limit);

}  // namespace utils::log

USERVER_NAMESPACE_END
