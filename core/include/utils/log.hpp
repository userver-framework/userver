#pragma once

#include <string_view>

namespace utils::log {

/// returns `data` converted to hex and truncated to `limit`
std::string ToLimitedHex(std::string_view data, size_t limit);

/// if `data` in utf-8, returns `data` truncated to `limit`
/// otherwise returns stub
std::string ToLimitedUtf8(std::string_view data, size_t limit);

}  // namespace utils::log
