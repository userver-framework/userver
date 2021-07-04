#pragma once

/// @file userver/utils/assert.hpp
/// @brief Assertion macros UASSERT, UASSERT_MSG, YTX_INVARIANT

#include <string>

#include <fmt/format.h>

#include <userver/utils/traceful_exception.hpp>

#if !defined(NDEBUG) && !defined(DOXYGEN)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT(expr)                                                   \
  do {                                                                  \
    if (!(expr)) {                                                      \
      ::utils::UASSERT_failed(#expr, __FILE__, __LINE__, __func__, ""); \
    }                                                                   \
  } while (0)

// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT_MSG(expr, msg)                                           \
  do {                                                                   \
    if (!(expr)) {                                                       \
      ::utils::UASSERT_failed(#expr, __FILE__, __LINE__, __func__, msg); \
    }                                                                    \
  } while (0)

namespace utils {

[[noreturn]] void UASSERT_failed(const std::string& expr, const char* file,
                                 unsigned int line, const char* function,
                                 const std::string& msg) noexcept;
}  // namespace utils

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

namespace utils {

class InvariantError : public TracefulException {
  using TracefulException::TracefulException;
};

[[noreturn]] void LogAndThrowInvariantError(const std::string& error);

/// @brief Asserts in debug builds, throws utils::InvariantError in release
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define YTX_INVARIANT(condition, message)                                     \
  do {                                                                        \
    if (!(condition)) {                                                       \
      const auto err_str =                                                    \
          ::fmt::format("Invariant ({}) violation: {}", #condition, message); \
      UASSERT_MSG(false, err_str);                                            \
      ::utils::LogAndThrowInvariantError(err_str);                            \
    }                                                                         \
  } while (0)

}  // namespace utils
