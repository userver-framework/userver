#pragma once

#include <string>
#include <string_view>

/// @file userver/utils/thread_name.hpp
/// @brief Functions for thread name manipulation

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Get the name of the current thread
///
/// Thread names should only be used for debugging purposes. Thread names aren't
/// required to be preserved exactly due to the system limit on thread name
/// length.
///
/// @returns the current thread name
/// @throws std::system_error on error
std::string GetCurrentThreadName();

/// @brief Set the name of the current thread
///
/// This function should *only* be used from drivers that create their own
/// threads! Users should normally rely on the coroutine engine instead of
/// creating additional threads.
///
/// There is a system limit on thread name length, e.g. 15 chars on Linux.
/// The name is automatically cut to fit the limit.
///
/// @param name the new thread name
/// @throws std::system_error on error
void SetCurrentThreadName(std::string_view name);

}  // namespace utils

USERVER_NAMESPACE_END
