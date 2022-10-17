#pragma once

#include <chrono>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

/// @brief The function to format provided timepoint to http Date format
/// (with timezone being UTC).
///
/// @note You are not expected to use this function directly, it's placed
/// in headers for benchmarking purposes only.
std::string MakeHttpDate(std::chrono::system_clock::time_point date);

}  // namespace impl

/// @brief Returns string_view of http-formatted current date (with timezone
/// being UTC).
///
/// @note Resulting string_view should not cross thread boundaries, otherwise
/// it's UB.
std::string_view GetCachedDate();

}  // namespace server::http

USERVER_NAMESPACE_END
