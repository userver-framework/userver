#pragma once

/// @file userver/utils/assert.hpp
/// @brief Assertion macros UASSERT, UASSERT_MSG, UINVARIANT and AbortWithStacktrace() function
/// @ingroup userver_universal

#include <string_view>

#include <userver/utils/impl/source_location.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

[[noreturn]] void UASSERT_failed(
    std::string_view expr,
    const char* file,
    unsigned int line,
    const char* function,
    std::string_view msg
) noexcept;

[[noreturn]] void LogAndThrowInvariantError(
    std::string_view condition,
    std::string_view message,
    utils::impl::SourceLocation source_location
);

#ifdef NDEBUG
inline constexpr bool kEnableAssert = false;
#else
inline constexpr bool kEnableAssert = true;
#endif

extern bool dump_stacktrace_on_assert_failure;

}  // namespace impl

/// @brief Function that prints the stacktrace with message and aborts the program execution.
///
/// Mostly useful for placing in dead code branches or in 'should-never-happen-and-theres-no-way-to-restore' places.
[[noreturn]] void AbortWithStacktrace(std::string_view message) noexcept;

}  // namespace utils

USERVER_NAMESPACE_END

/// @brief Assertion macro for that aborts execution in DEBUG builds with a
/// message `msg` and does nothing in release builds
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT_MSG(expr, msg)                                                                            \
    do {                                                                                                  \
        if (USERVER_NAMESPACE::utils::impl::kEnableAssert) {                                              \
            if (expr) {                                                                                   \
            } else {                                                                                      \
                USERVER_NAMESPACE::utils::impl::UASSERT_failed(#expr, __FILE__, __LINE__, __func__, msg); \
            }                                                                                             \
        }                                                                                                 \
    } while (0)

/// @brief Assertion macro that aborts execution in DEBUG builds and does
/// nothing in release builds
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UASSERT(expr) UASSERT_MSG(expr, std::string_view{})

/// @brief Asserts in debug builds, throws utils::InvariantError in release
///
/// @hideinitializer
// NOLINTNEXTLINE (cppcoreguidelines-macro-usage)
#define UINVARIANT(condition, message)                                                                             \
    do {                                                                                                           \
        if (condition) {                                                                                           \
        } else {                                                                                                   \
            if constexpr (USERVER_NAMESPACE::utils::impl::kEnableAssert) {                                         \
                USERVER_NAMESPACE::utils::impl::UASSERT_failed(#condition, __FILE__, __LINE__, __func__, message); \
            } else {                                                                                               \
                USERVER_NAMESPACE::utils::impl::LogAndThrowInvariantError(                                         \
                    #condition, message, USERVER_NAMESPACE::utils::impl::SourceLocation::Current()                 \
                );                                                                                                 \
            }                                                                                                      \
        }                                                                                                          \
    } while (0)
