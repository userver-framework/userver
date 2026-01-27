#pragma once

/// @file userver/logging/log.hpp
/// @brief Logging helpers, see @ref scripts/docs/en/userver/logging.md for more info.

#include <chrono>

#include <userver/compiler/select.hpp>
#include <userver/logging/fwd.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log_filepath.hpp>
#include <userver/logging/log_helper.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

void SetDefaultLoggerRef(LoggerRef new_logger) noexcept;

extern bool has_background_threads_which_can_log;

}  // namespace impl

/// @brief Returns the default logger previously set by SetDefaultLogger. If the
/// logger was not set - returns a logger that does no logging.
///
/// @note While the coroutine engine is running, any reference to the default
/// logger is guaranteed to be alive. No lifetime guarantees are given
/// for the default logger reference outside the engine's lifetime. The rule of
/// thumb there is not to keep this reference in any extended scope.
LoggerRef GetDefaultLogger() noexcept;

/// @brief Atomically replaces the default logger.
///
/// @warning Do not use this class if you are using a component system.
class DefaultLoggerGuard final {
public:
    /// Atomically replaces the default logger.
    ///
    /// @warning The logger should live as long as someone is using it.
    /// Generally speaking - it should live for a lifetime of the application,
    /// or for a lifetime of the coroutine engine.
    explicit DefaultLoggerGuard(LoggerPtr new_default_logger) noexcept;

    DefaultLoggerGuard(DefaultLoggerGuard&&) = delete;
    DefaultLoggerGuard& operator=(DefaultLoggerGuard&&) = delete;

    ~DefaultLoggerGuard();

private:
    LoggerRef logger_prev_;
    const Level level_prev_;
    LoggerPtr logger_new_;
};

/// @brief Allows to override global log level for the whole service within a scope. Primarily for use in tests.
///
/// @warning This is NOT the right tool to toggle writing of certain logs within a scope.
/// This scope class changes log level GLOBALLY as-if using @ref logging::SetLoggerLevel.
///
/// @note To affect what logs are written within a scope, use @ref tracing::Span::SetLogLevel and
/// @ref tracing::Span::SetLocalLogLevel (read their docs first!).
class DefaultLoggerLevelScope final {
public:
    explicit DefaultLoggerLevelScope(logging::Level level);

    DefaultLoggerLevelScope(DefaultLoggerLevelScope&&) = delete;
    DefaultLoggerLevelScope& operator=(DefaultLoggerLevelScope&&) = delete;

    ~DefaultLoggerLevelScope();

private:
    impl::LoggerBase& logger_;
    const Level level_initial_;
};

/// @brief Sets new log level for the default logger
/// @note Prefer using logging::DefaultLoggerLevelScope if possible
void SetDefaultLoggerLevel(Level);

/// Returns log level for the default logger
Level GetDefaultLoggerLevel() noexcept;

/// Returns true if the provided log level is greater or equal to
/// the current log level and to the tracing::Span (if any) local log level.
bool ShouldLog(Level level) noexcept;

/// Sets new log level for a logger
void SetLoggerLevel(LoggerRef, Level);

bool LoggerShouldLog(LoggerRef logger, Level level) noexcept;

bool LoggerShouldLog(const LoggerPtr& logger, Level level) noexcept;

Level GetLoggerLevel(LoggerRef logger) noexcept;

/// Forces flush of default logger message queue
void LogFlush();

/// Forces flush of `logger` message queue
void LogFlush(LoggerRef logger);

namespace impl {

// Not thread-safe, static lifetime data
class RateLimitData {
public:
    uint64_t count_since_reset = 0;
    uint64_t dropped_count = 0;
    std::chrono::steady_clock::time_point last_reset_time{};
};

// Represents a single rate limit usage
class RateLimiter {
public:
    explicit RateLimiter(RateLimitData& data) noexcept;
    bool ShouldLog() const noexcept { return should_log_; }
    auto GetDroppedCount() const noexcept { return dropped_count_; }
    friend LogHelper& operator<<(LogHelper& lh, const RateLimiter& rl) noexcept;

private:
    bool should_log_{true};
    uint64_t dropped_count_{0};
};

// Register location during static initialization for dynamic debug logs.
class StaticLogEntry final {
public:
    StaticLogEntry(const char* path, int line) noexcept;

    StaticLogEntry(StaticLogEntry&&) = delete;
    StaticLogEntry& operator=(StaticLogEntry&&) = delete;

    bool ShouldNotLog(LoggerRef logger, Level level) const noexcept;
    bool ShouldNotLog(const LoggerPtr& logger, Level level) const noexcept;

private:
    static constexpr std::size_t kContentSize = compiler::SelectSize().For64Bit(40).For32Bit(24);

    alignas(void*) std::byte content_[kContentSize];
};

template <class NameHolder, int Line>
struct EntryStorage final {
    static inline StaticLogEntry entry{NameHolder::Get(), Line};
};

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

/// @cond

#ifdef USERVER_FEATURE_ERASE_LOG_WITH_LEVEL

// Helper macro to erase the logging related code from binary. Erases the
// * logging registration via EntryStorage
// * ShouldLog() calls and related `if` statements and runtime checks
// * SourceLocation info
#define USERVER_IMPL_ERASE_LOG(logger, ...)                                     \
    true                                                                        \
        ? logging::impl::Noop{}                                                 \
        : USERVER_NAMESPACE::logging::LogHelper(                                \
              logger,                                                           \
              USERVER_NAMESPACE::logging::Level::kTrace,                        \
              USERVER_NAMESPACE::logging::LogClass::kLog,                       \
              USERVER_NAMESPACE::utils::impl::SourceLocation::Custom(0, {}, {}) \
          )                                                                     \
              .AsLvalue(__VA_ARGS__)

#define USERVER_IMPL_LOGS_TRACE_ERASER(MACRO, LOGGER, ...) USERVER_IMPL_ERASE_LOG(LOGGER, __VA_ARGS__)

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 0
#define USERVER_IMPL_LOGS_DEBUG_ERASER(MACRO, LOGGER, ...) USERVER_IMPL_ERASE_LOG(LOGGER, __VA_ARGS__)
#endif

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 1
#define USERVER_IMPL_LOGS_INFO_ERASER(MACRO, LOGGER, ...) USERVER_IMPL_ERASE_LOG(LOGGER, __VA_ARGS__)
#endif

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 2
#define USERVER_IMPL_LOGS_WARNING_ERASER(MACRO, LOGGER, ...) USERVER_IMPL_ERASE_LOG(LOGGER, __VA_ARGS__)
#endif

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 3
#define USERVER_IMPL_LOGS_ERROR_ERASER(MACRO, LOGGER, ...) USERVER_IMPL_ERASE_LOG(LOGGER, __VA_ARGS__)
#endif

#endif  // #ifdef USERVER_FEATURE_ERASE_LOG_WITH_LEVEL

#ifndef USERVER_IMPL_LOGS_TRACE_ERASER
#define USERVER_IMPL_LOGS_TRACE_ERASER(MACRO, LOGGER, ...) \
    MACRO(LOGGER, USERVER_NAMESPACE::logging::Level::kTrace, __VA_ARGS__)
#endif

#ifndef USERVER_IMPL_LOGS_DEBUG_ERASER
#define USERVER_IMPL_LOGS_DEBUG_ERASER(MACRO, LOGGER, ...) \
    MACRO(LOGGER, USERVER_NAMESPACE::logging::Level::kDebug, __VA_ARGS__)
#endif

#ifndef USERVER_IMPL_LOGS_INFO_ERASER
#define USERVER_IMPL_LOGS_INFO_ERASER(MACRO, LOGGER, ...) \
    MACRO(LOGGER, USERVER_NAMESPACE::logging::Level::kInfo, __VA_ARGS__)
#endif

#ifndef USERVER_IMPL_LOGS_WARNING_ERASER
#define USERVER_IMPL_LOGS_WARNING_ERASER(MACRO, LOGGER, ...) \
    MACRO(LOGGER, USERVER_NAMESPACE::logging::Level::kWarning, __VA_ARGS__)
#endif

#ifndef USERVER_IMPL_LOGS_ERROR_ERASER
#define USERVER_IMPL_LOGS_ERROR_ERASER(MACRO, LOGGER, ...) \
    MACRO(LOGGER, USERVER_NAMESPACE::logging::Level::kError, __VA_ARGS__)
#endif

#define USERVER_IMPL_LOG_TO(logger, level, ...)                                                      \
    USERVER_NAMESPACE::logging::LogHelper(logger, level, USERVER_NAMESPACE::logging::LogClass::kLog) \
        .AsLvalue(__VA_ARGS__)

#define USERVER_IMPL_DYNAMIC_DEBUG_ENTRY                                                                 \
    []() noexcept -> const USERVER_NAMESPACE::logging::impl::StaticLogEntry& {                           \
        struct NameHolder {                                                                              \
            static constexpr const char* Get() noexcept { return USERVER_FILEPATH.c_str(); }             \
        };                                                                                               \
        const auto& entry = USERVER_NAMESPACE::logging::impl::EntryStorage<NameHolder, __LINE__>::entry; \
        return entry;                                                                                    \
    }
/// @endcond

/// @brief If `lvl` matches the verbosity then builds a stream and evaluates a
/// message for the specified `logger`.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
/// @hideinitializer
// static_cast<int> below are workarounds for -Wtautological-compare
#define LOG_TO(logger, lvl, ...)                                                           \
    __builtin_expect(                                                                      \
        USERVER_IMPL_DYNAMIC_DEBUG_ENTRY().ShouldNotLog((logger), (lvl)),                  \
        static_cast<int>(lvl) < static_cast<int>(USERVER_NAMESPACE::logging::Level::kInfo) \
    )                                                                                      \
        ? USERVER_NAMESPACE::logging::impl::Noop{}                                         \
        : USERVER_IMPL_LOG_TO((logger), (lvl), __VA_ARGS__)

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
/// @hideinitializer
#define LOG(lvl, ...) LOG_TO(USERVER_NAMESPACE::logging::GetDefaultLogger(), (lvl), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to @ref logging::Level::kTrace
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_TRACE(...) \
    USERVER_IMPL_LOGS_TRACE_ERASER(LOG_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to @ref logging::Level::kDebug
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_DEBUG(...) \
    USERVER_IMPL_LOGS_DEBUG_ERASER(LOG_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to @ref logging::Level::kInfo
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_INFO(...) USERVER_IMPL_LOGS_INFO_ERASER(LOG_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to @ref logging::Level::kWarning
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_WARNING(...) \
    USERVER_IMPL_LOGS_WARNING_ERASER(LOG_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to @ref logging::Level::kError
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_ERROR(...) \
    USERVER_IMPL_LOGS_ERROR_ERASER(LOG_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to @ref logging::Level::kCritical
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_CRITICAL(...) LOG(USERVER_NAMESPACE::logging::Level::kCritical, __VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to @ref logging::Level::kTrace
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_TRACE_TO(logger, ...) USERVER_IMPL_LOGS_TRACE_ERASER(LOG_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to @ref logging::Level::kDebug
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_DEBUG_TO(logger, ...) USERVER_IMPL_LOGS_DEBUG_ERASER(LOG_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to @ref logging::Level::kInfo
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_INFO_TO(logger, ...) USERVER_IMPL_LOGS_INFO_ERASER(LOG_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to @ref logging::Level::kWarning
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_WARNING_TO(logger, ...) USERVER_IMPL_LOGS_WARNING_ERASER(LOG_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to @ref logging::Level::kError
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_ERROR_TO(logger, ...) USERVER_IMPL_LOGS_ERROR_ERASER(LOG_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to @ref logging::Level::kCritical
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_CRITICAL_TO(logger, ...) LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kCritical, __VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the logger. Ignores log messages that occur too often.
/// @hideinitializer
#define LOG_LIMITED_TO(logger, lvl, ...)                                                         \
    if (const USERVER_NAMESPACE::logging::impl::RateLimiter                                      \
            userver_log_limited_to_rl{[]() -> USERVER_NAMESPACE::logging::impl::RateLimitData& { \
                thread_local USERVER_NAMESPACE::logging::impl::RateLimitData rl_data;            \
                return rl_data;                                                                  \
            }()};                                                                                \
        !userver_log_limited_to_rl.ShouldLog())                                                  \
    {                                                                                            \
    } else                                                                                       \
        LOG_TO((logger), (lvl), __VA_ARGS__) << userver_log_limited_to_rl

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger. Ignores log messages that occur too often.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED(lvl, ...) LOG_LIMITED_TO(USERVER_NAMESPACE::logging::GetDefaultLogger(), (lvl), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to @ref logging::Level::kTrace.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_TRACE(...) \
    USERVER_IMPL_LOGS_TRACE_ERASER(LOG_LIMITED_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to @ref logging::Level::kDebug.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_DEBUG(...) \
    USERVER_IMPL_LOGS_DEBUG_ERASER(LOG_LIMITED_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to @ref logging::Level::kInfo.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_INFO(...) \
    USERVER_IMPL_LOGS_INFO_ERASER(LOG_LIMITED_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to @ref logging::Level::kWarning.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_WARNING(...) \
    USERVER_IMPL_LOGS_WARNING_ERASER(LOG_LIMITED_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to @ref logging::Level::kError.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_ERROR(...) \
    USERVER_IMPL_LOGS_ERROR_ERASER(LOG_LIMITED_TO, USERVER_NAMESPACE::logging::GetDefaultLogger(), __VA_ARGS__)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to @ref logging::Level::kCritical.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_CRITICAL(...) LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kCritical, __VA_ARGS__)

///////////////////////////////////////////////////////////////////////////////

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to @ref logging::Level::kTrace.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_TRACE_TO(logger, ...) USERVER_IMPL_LOGS_TRACE_ERASER(LOG_LIMITED_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to @ref logging::Level::kDebug.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_DEBUG_TO(logger, ...) USERVER_IMPL_LOGS_DEBUG_ERASER(LOG_LIMITED_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to @ref logging::Level::kInfo.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_INFO_TO(logger, ...) USERVER_IMPL_LOGS_INFO_ERASER(LOG_LIMITED_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to @ref logging::Level::kWarning.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_WARNING_TO(logger, ...) USERVER_IMPL_LOGS_WARNING_ERASER(LOG_LIMITED_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to @ref logging::Level::kError.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_ERROR_TO(logger, ...) USERVER_IMPL_LOGS_ERROR_ERASER(LOG_LIMITED_TO, logger, __VA_ARGS__)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to @ref logging::Level::kCritical.
///
/// @param ... optional `fmt::format` string literal and its arguments
///
/// Not affected by `USERVER_FEATURE_ERASE_LOG_WITH_LEVEL` @ref scripts/docs/en/userver/build/options.md "CMake option".
#define LOG_LIMITED_CRITICAL_TO(logger, ...) \
    LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kCritical, __VA_ARGS__)

// NOLINTEND(cppcoreguidelines-macro-usage)
