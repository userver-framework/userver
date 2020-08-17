#pragma once

#include <string>

namespace utils::log {

/// returns `data` converted to hex and truncated to `limit`
std::string ToLimitedHex(const std::string& data, size_t limit);

/// if `data` in utf-8, returns `data` truncated to `limit`
/// otherwise returns stub
std::string ToLimitedUtf8(const std::string& data, size_t limit);

}  // namespace utils::log
