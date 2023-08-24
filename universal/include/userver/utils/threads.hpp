#pragma once

/// @file userver/utils/threads.hpp
/// @brief Functions to work with OS threads.
/// @ingroup userver_universal

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @returns true if this is the thread in which main() was started
bool IsMainThread() noexcept;

/// @brief Set priority of the OS thread to IDLE (the lowest one)
/// @throws std::system_error
void SetCurrentThreadIdleScheduling();

/// @brief Set priority of the OS thread to low (but not the lowest one)
/// @throws std::system_error
void SetCurrentThreadLowPriorityScheduling();

}  // namespace utils

USERVER_NAMESPACE_END
