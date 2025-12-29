#pragma once

/// @file userver/utils/debugging.hpp
/// @brief Utility for runtime debugger detection

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Checks whether a debugger is attached to the current process.
///
/// This function offers a cross-platform method for detecting the presence of a debugger.
/// - If C++26's `__cpp_lib_debugging` feature is available, it calls `std::is_debugger_present()` directly.
/// - If not, it performs platform-specific checks to make this determination.
///
/// Supported platforms: **Linux** and **Apple**.
/// On unsupported platforms, this function returns `false`.
///
/// @return `true` if a debugger is attached to the process, `false` otherwise.
bool IsDebuggerPresent() noexcept;

}  // namespace utils

USERVER_NAMESPACE_END
