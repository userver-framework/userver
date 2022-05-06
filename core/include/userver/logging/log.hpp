#pragma once

/// @file userver/logging/log.hpp
/// @brief Logging helpers

#include <chrono>

#include <userver/logging/level.hpp>
#include <userver/logging/log_filepath.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/logging/logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

/// Returns default logger
LoggerPtr DefaultLogger();

/// Returns default logger or a nullptr if there are problems with memory
/// allocation
LoggerPtr DefaultLoggerOptional() noexcept;

/// Atomically replaces default logger and returns the old one
LoggerPtr SetDefaultLogger(LoggerPtr);

/// Sets new log level for default logger
void SetDefaultLoggerLevel(Level);

void SetLoggerLevel(LoggerPtr, Level);

/// Returns log level for default logger
Level GetDefaultLoggerLevel();

bool LoggerShouldLog(const LoggerPtr& logger, Level level);

Level GetLoggerLevel(const LoggerPtr& logger);

/// Forces flush of default logger message queue
void LogFlush();

/// Forces flush of `logger` message queue
void LogFlush(LoggerPtr logger);

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
  RateLimiter(LoggerPtr logger, RateLimitData& data, Level level) noexcept;
  bool ShouldLog() const { return should_log_; }
  void SetShouldNotLog() { should_log_ = false; }
  Level GetLevel() const { return level_; }
  friend LogHelper& operator<<(LogHelper& lh, const RateLimiter& rl) noexcept;

 private:
  LoggerPtr logger_;
  const Level level_;
  bool should_log_;
  uint64_t dropped_count_;
};

/*
 * Statically initialized list of LOG_XXX() locations.
 * It shall be used for dynamic debug logs.
 */
struct LogEntry {
  explicit LogEntry(std::string_view name);

  std::string_view name;
  const LogEntry* next;
};

inline const LogEntry* entry_slist{nullptr};

inline LogEntry::LogEntry(std::string_view name)
    : name(name), next(entry_slist) {
  // ctr adds itself to the list
  entry_slist = this;
}

template <const std::string_view& Name>
struct EntryStorage {
  static inline const LogEntry entry{Name};
};

}  // namespace impl

}  // namespace logging

/// @brief Builds a stream and evaluates a message for the logger.
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DO_LOG_TO(logger, lvl)                                         \
  USERVER_NAMESPACE::logging::LogHelper(logger, lvl, USERVER_FILEPATH, \
                                        __LINE__, __func__)            \
      .AsLvalue()

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DO_USERVER_IMPL_STR(x) #x
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_STR(x) DO_USERVER_IMPL_STR(x)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_LOCATION __FILE__ ":" USERVER_IMPL_STR(__LINE__)

// static_cast<int> below are workarounds for clangs -Wtautological-compare

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger.
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG(lvl)                                                               \
  __builtin_expect(                                                            \
      !USERVER_NAMESPACE::logging::ShouldLog(                                  \
          lvl,                                                                 \
          []() -> std::string_view {                                           \
            static constexpr std::string_view name_ = (USERVER_IMPL_LOCATION); \
            (void)                                                             \
                USERVER_NAMESPACE::logging::impl::EntryStorage<name_>::entry;  \
            return name_;                                                      \
          }()),                                                                \
      static_cast<int>(lvl) <                                                  \
          static_cast<int>(USERVER_NAMESPACE::logging::Level::kInfo))          \
      ? USERVER_NAMESPACE::logging::impl::Noop{}                               \
      : DO_LOG_TO(USERVER_NAMESPACE::logging::DefaultLoggerOptional(), (lvl))

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger.
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TO(logger, lvl)                                     \
  !USERVER_NAMESPACE::logging::LoggerShouldLog((logger), (lvl)) \
      ? USERVER_NAMESPACE::logging::impl::Noop{}                \
      : DO_LOG_TO((logger), (lvl))

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kTrace
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE() LOG(USERVER_NAMESPACE::logging::Level::kTrace)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kDebug
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG() LOG(USERVER_NAMESPACE::logging::Level::kDebug)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kInfo
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO() LOG(USERVER_NAMESPACE::logging::Level::kInfo)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kWarning
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING() LOG(USERVER_NAMESPACE::logging::Level::kWarning)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kError
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR() LOG(USERVER_NAMESPACE::logging::Level::kError)

/// @brief Evaluates a message and logs it to the default logger if its level is
/// below or equal to logging::Level::kCritical
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_CRITICAL() LOG(USERVER_NAMESPACE::logging::Level::kCritical)

///////////////////////////////////////////////////////////////////////////////

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kTrace
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE_TO(logger) \
  LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kTrace)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kDebug
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG_TO(logger) \
  LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kDebug)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kInfo
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO_TO(logger) \
  LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kInfo)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kWarning
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING_TO(logger) \
  LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kWarning)

/// @brief Evaluates a message and logs it to the `logger` if its level is below
/// or equal to logging::Level::kError
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR_TO(logger) \
  LOG_TO(logger, USERVER_NAMESPACE::logging::Level::kError)

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
           logger,                                                         \
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
  LOG_LIMITED_TO(USERVER_NAMESPACE::logging::DefaultLoggerOptional(), lvl)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kTrace.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_TRACE() \
  LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kTrace)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kDebug.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_DEBUG() \
  LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kDebug)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kInfo.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_INFO() LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kInfo)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kWarning.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_WARNING() \
  LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kWarning)

/// @brief Evaluates a message and logs it to the default logger if the log
/// message does not occur too often and default logger level is below or equal
/// to logging::Level::kError.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_ERROR() \
  LOG_LIMITED(USERVER_NAMESPACE::logging::Level::kError)

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
  LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kTrace)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kDebug.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_DEBUG_TO(logger) \
  LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kDebug)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kInfo.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_INFO_TO(logger) \
  LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kInfo)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kWarning.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_WARNING_TO(logger) \
  LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kWarning)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kError.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_ERROR_TO(logger) \
  LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kError)

/// @brief Evaluates a message and logs it to the `logger` if the log message
/// does not occur too often and `logger` level is below or equal to
/// logging::Level::kCritical.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_CRITICAL_TO(logger) \
  LOG_LIMITED_TO(logger, USERVER_NAMESPACE::logging::Level::kCritical)

USERVER_NAMESPACE_END
