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

/// @brief RAII helper to run some code with a temporary current thread name
///
/// This might be useful for spawning external library threads if no other
/// customisation points are available. All restrictions for thread name
/// operations apply.
///
/// @note There is a chance of failure during thread name update on scope exit.
/// No errors will be reported in this case.
class CurrentThreadNameGuard {
 public:
  /// @brief Set the name of the current thread in the current scope
  /// @param name temporary thread name
  /// @throws std::system_error on error
  CurrentThreadNameGuard(std::string_view name);
  ~CurrentThreadNameGuard();

  CurrentThreadNameGuard(const CurrentThreadNameGuard&) = delete;
  CurrentThreadNameGuard(CurrentThreadNameGuard&&) noexcept = delete;
  CurrentThreadNameGuard& operator=(const CurrentThreadNameGuard&) = delete;
  CurrentThreadNameGuard& operator=(CurrentThreadNameGuard&&) noexcept = delete;

 private:
  std::string prev_name_;
};

}  // namespace utils

USERVER_NAMESPACE_END
