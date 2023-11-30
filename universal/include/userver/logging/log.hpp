#pragma once

/// @file userver/logging/log.hpp
/// @brief Logging helpers

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

/// @brief Allows to override default log level within a scope. Primarily for
/// use in tests.
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
  RateLimiter(RateLimitData& data, Level level) noexcept;
  bool ShouldLog() const { return should_log_; }
  void SetShouldNotLog() { should_log_ = false; }
  Level GetLevel() const { return level_; }
  friend LogHelper& operator<<(LogHelper& lh, const RateLimiter& rl) noexcept;

 private:
  const Level level_;
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
  static constexpr std::size_t kContentSize =
      compiler::SelectSize().For64Bit(40).For32Bit(24);

  alignas(void*) std::byte content_[kContentSize];
};

template <class NameHolder, int Line>
struct EntryStorage final {
  static inline StaticLogEntry entry{NameHolder::Get(), Line};
};

}  // namespace impl

}  // namespace logging

/// @cond

#ifdef USERVER_FEATURE_ERASE_LOG_WITH_LEVEL

// Helper macro to erase the logging related code from binary. Erases the
// * logging registration via EntryStorage
// * ShouldLog() calls and related `if` statements and runtime checks
// * SourceLocation info
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_ERASE_LOG                                                 \
  true                                                                         \
      ? logging::impl::Noop{}                                                  \
      : USERVER_NAMESPACE::logging::LogHelper(                                 \
            USERVER_NAMESPACE::logging::GetDefaultLogger(),                    \
            USERVER_NAMESPACE::logging::Level::kTrace,                         \
            USERVER_NAMESPACE::utils::impl::SourceLocation::Custom(0, {}, {})) \
            .AsLvalue()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_TRACE_ERASER(X) USERVER_IMPL_ERASE_LOG

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 0
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_DEBUG_ERASER(X) USERVER_IMPL_ERASE_LOG
#endif

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 1
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_INFO_ERASER(X) USERVER_IMPL_ERASE_LOG
#endif

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 2
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_WARNING_ERASER(X) USERVER_IMPL_ERASE_LOG
#endif

#if USERVER_FEATURE_ERASE_LOG_WITH_LEVEL > 3
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_ERROR_ERASER(X) USERVER_IMPL_ERASE_LOG
#endif

#endif  // #ifdef USERVER_FEATURE_ERASE_LOG_WITH_LEVEL

#ifndef USERVER_IMPL_LOGS_TRACE_ERASER
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_TRACE_ERASER(X) X
#endif

#ifndef USERVER_IMPL_LOGS_DEBUG_ERASER
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_DEBUG_ERASER(X) X
#endif

#ifndef USERVER_IMPL_LOGS_INFO_ERASER
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_INFO_ERASER(X) X
#endif

#ifndef USERVER_IMPL_LOGS_WARNING_ERASER
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_WARNING_ERASER(X) X
#endif

#ifndef USERVER_IMPL_LOGS_ERROR_ERASER
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOGS_ERROR_ERASER(X) X
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOG_TO(logger, level) \
  USERVER_NAMESPACE::logging::LogHelper(logger, level).AsLvalue()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_DYNAMIC_DEBUG_ENTRY                                     \
  []() noexcept -> const USERVER_NAMESPACE::logging::impl::StaticLogEntry& { \
    struct NameHolder {                                                      \
      static constexpr const char* Get() noexcept {                          \
        return USERVER_FILEPATH.data();                                      \
      }                                                                      \
    };                                                                       \
    const auto& entry =                                                      \
        USERVER_NAMESPACE::logging::impl::EntryStorage<NameHolder,           \
                                                       __LINE__>::entry;     \
    return entry;                                                            \
  }
/// @endcond

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the specified logger.
/// @hideinitializer
// static_cast<int> below are workarounds for -Wtautological-compare
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TO(logger, lvl)                                             \
  __builtin_expect(                                                     \
      USERVER_IMPL_DYNAMIC_DEBUG_ENTRY().ShouldNotLog((logger), (lvl)), \
      static_cast<int>(lvl) <                                           \
          static_cast<int>(USERVER_NAMESPACE::logging::Level::kInfo))   \
      ? USERVER_NAMESPACE::logging::impl::Noop{}                        \
      : USERVER_IMPL_LOG_TO((logger), (lvl))

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger.
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG(lvl) LOG_TO(USERVER_NAMESPACE::logging::GetDefaultLogger(), (lvl))

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kTrace
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE() \
  USERVER_IMPL_LOGS_TRACE_ERASER(LOG(USERVER_NAMESPACE::logging::Level::kTrace))

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kDebug
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG() \
  USERVER_IMPL_LOGS_DEBUG_ERASER(LOG(USERVER_NAMESPACE::logging::Level::kDebug))

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kInfo
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO() \
  USERVER_IMPL_LOGS_INFO_ERASER(LOG(USERVER_NAMESPACE::logging::Level::kInfo))

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kWarning
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING()               \
  USERVER_IMPL_LOGS_WARNING_ERASER( \
      LOG(USERVER_NAMESPACE::logging::Level::kWarning))

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kError
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR() \
  USERVER_IMPL_LOGS_ERROR_ERASER(LOG(USERVER_NAMESPACE::logging::Level::kError))

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kCritical
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_CRITICAL() LOG(USERVER_NAMESPACE::logging::Level::kCritical)

///////////////////////////////////////////////////////////////////////////////

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kTrace
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE_TO(logger)      \
  USERVER_IMPL_LOGS_TRACE_ERASER( \
      LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kTrace))

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kDebug
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG_TO(logger)      \
  USERVER_IMPL_LOGS_DEBUG_ERASER( \
      LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kDebug))

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kInfo
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO_TO(logger)      \
  USERVER_IMPL_LOGS_INFO_ERASER( \
      LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kInfo))

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kWarning
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING_TO(logger)      \
  USERVER_IMPL_LOGS_WARNING_ERASER( \
      LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kWarning))

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kError
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR_TO(logger)      \
  USERVER_IMPL_LOGS_ERROR_ERASER( \
      LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kError))

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kCritical
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_CRITICAL_TO(logger) \
  LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kCritical)

///////////////////////////////////////////////////////////////////////////////

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the logger. Ignores log messages that occur too often.
/// @hideinitializer
// Note: we have to jump through the hoops to keep lazy evaluation of the logged
// data AND log the dropped logs count from the correct LogHelper in the face of
// multithreading and coroutines.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_TO(logger, lvl)                                        \
  for (USERVER_NAMESPACE::logging::impl::RateLimiter log_limited_to_rl{    \
           []() -> USERVER_NAMESPACE::logging::impl::RateLimitData& {      \
             thread_local USERVER_NAMESPACE::logging::impl::RateLimitData  \
                 rl_data;                                                  \
             return rl_data;                                               \
           }(),                                                            \
           (lvl)};                                                         \
       log_limited_to_rl.ShouldLog(); log_limited_to_rl.SetShouldNotLog()) \
  LOG_TO((logger), log_limited_to_rl.GetLevel()) << log_limited_to_rl

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger. Ignores log messages that occur too often.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED(lvl) \
  LOG_LIMITED_TO(USERVER_NAMESPACE::logging::GetDefaultLogger(), lvl)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kTrace.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_TRACE()       \
  USERVER_IMPL_LOGS_TRACE_ERASER( \
      LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kTrace))

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kDebug.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_DEBUG()       \
  USERVER_IMPL_LOGS_DEBUG_ERASER( \
      LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kDebug))

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kInfo.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_INFO()       \
  USERVER_IMPL_LOGS_INFO_ERASER( \
      LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kInfo))

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kWarning.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_WARNING()       \
  USERVER_IMPL_LOGS_WARNING_ERASER( \
      LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kWarning))

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kError.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_ERROR()       \
  USERVER_IMPL_LOGS_ERROR_ERASER( \
      LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kError))

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kCritical.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_CRITICAL() \
  LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kCritical)

///////////////////////////////////////////////////////////////////////////////

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kTrace.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_TRACE_TO(logger) \
  USERVER_IMPL_LOGS_TRACE_ERASER(    \
      LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kTrace))

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kDebug.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_DEBUG_TO(logger) \
  USERVER_IMPL_LOGS_DEBUG_ERASER(    \
      LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kDebug))

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kInfo.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_INFO_TO(logger) \
  USERVER_IMPL_LOGS_INFO_ERASER(    \
      LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kInfo))

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kWarning.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_WARNING_TO(logger) \
  USERVER_IMPL_LOGS_WARNING_ERASER(    \
      LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kWarning))

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kError.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_ERROR_TO(logger) \
  USERVER_IMPL_LOGS_ERROR_ERASER(    \
      LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kError))

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kCritical.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_CRITICAL_TO(logger) \
  LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kCritical)

USERVER_NAMESPACE_END
