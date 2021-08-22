#pragma once

/// @file userver/utils/assert.hpp
/// @brief Assertion macros UASSERT, UASSERT_MSG, UINVARIANT

#include <string_view>

#if !defined(NDEBUG) && !defined(DOXYGEN)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT(expr)                                                         \
  do {                                                                        \
    if (!(expr)) {                                                            \
      ::utils::impl::UASSERT_failed(#expr, __FILE__, __LINE__, __func__, ""); \
    }                                                                         \
  } while (0)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT_MSG(expr, msg)                                                 \
  do {                                                                         \
    if (!(expr)) {                                                             \
      ::utils::impl::UASSERT_failed(#expr, __FILE__, __LINE__, __func__, msg); \
    }                                                                          \
  } while (0)

namespace utils::impl {

[[noreturn]] void UASSERT_failed(std::string_view expr, const char* file,
                                 unsigned int line, const char* function,
                                 std::string_view msg) noexcept;

}  // namespace utils::impl

#else  // NDEBUG

/// @brief Assertion macro that aborts execution in DEBUG builds and does
/// nothing in release builds
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT(expr)      \
  do {                     \
    if (false && (expr)) { \
    }                      \
  } while (0)

/// Assertion macro for that aborts execution in DEBUG builds with a message
/// `msg` and does nothing in release builds
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT_MSG(expr, msg) \
  do {                         \
    if (false && (expr)) {     \
      (void)(msg);             \
    }                          \
  } while (0)

#endif  // NDEBUG

namespace utils::impl {

[[noreturn]] void LogAndThrowInvariantError(std::string_view condition,
                                            std::string_view message);

}  // namespace utils::impl

/// @brief Asserts in debug builds, throws utils::InvariantError in release
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UINVARIANT(condition, message)                               \
  do {                                                               \
    if (!(condition)) {                                              \
      UASSERT_MSG(condition, message);                               \
      ::utils::impl::LogAndThrowInvariantError(#condition, message); \
    }                                                                \
  } while (0)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define YTX_INVARIANT(condition, message) UINVARIANT(condition, message)
