#pragma once

/// @file userver/logging/log.hpp
/// @brief Logging helpers

#include <chrono>

#include <userver/logging/level.hpp>
#include <userver/logging/log_filepath.hpp>
#include <userver/logging/log_helper.hpp>
#include <userver/logging/logger.hpp>

namespace logging {

/// Returns default logger
LoggerPtr DefaultLogger();

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
  RateLimiter(LoggerPtr logger, RateLimitData& data, Level level);
  bool ShouldLog() const { return should_log_; }
  void SetShouldNotLog() { should_log_ = false; }
  Level GetLevel() const { return level_; }
  friend LogHelper& operator<<(LogHelper& lh, const RateLimiter& rl);

 private:
  LoggerPtr logger_;
  const Level level_;
  bool should_log_;
  uint64_t dropped_count_;
};

}  // namespace impl

}  // namespace logging

/// @brief Builds a stream and evaluates a message for the logger.
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DO_LOG_TO(logger, lvl)                                            \
  ::logging::LogHelper(logger, lvl, USERVER_FILEPATH, __LINE__, __func__) \
      .AsLvalue()

// static_cast<int> below are workarounds for clangs -Wtautological-compare

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger.
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG(lvl)                                                         \
  __builtin_expect(                                                      \
      !::logging::ShouldLog(lvl),                                        \
      static_cast<int>(lvl) < static_cast<int>(::logging::Level::kInfo)) \
      ? ::logging::impl::Noop{}                                          \
      : DO_LOG_TO(::logging::DefaultLogger(), (lvl))

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger.
/// @hideinitializer
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TO(logger, lvl)                                              \
  !::logging::LoggerShouldLog((logger), (lvl)) ? ::logging::impl::Noop{} \
                                               : DO_LOG_TO((logger), (lvl))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE() LOG(::logging::Level::kTrace)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG() LOG(::logging::Level::kDebug)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO() LOG(::logging::Level::kInfo)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING() LOG(::logging::Level::kWarning)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR() LOG(::logging::Level::kError)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_CRITICAL() LOG(::logging::Level::kCritical)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_TRACE_TO(logger) LOG_TO(logger, ::logging::Level::kTrace)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_DEBUG_TO(logger) LOG_TO(logger, ::logging::Level::kDebug)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INFO_TO(logger) LOG_TO(logger, ::logging::Level::kInfo)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_WARNING_TO(logger) LOG_TO(logger, ::logging::Level::kWarning)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ERROR_TO(logger) LOG_TO(logger, ::logging::Level::kError)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_CRITICAL_TO(logger) LOG_TO(logger, ::logging::Level::kCritical)

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the logger. Ignores log messages that occur too often.
/// @hideinitializer
// Note: we have to jump through the hoops to keep lazy evaluation of the logged
// data AND log the dropped logs count from the correct LogHelper in the face of
// multithreading and coroutines.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_TO(logger, lvl)                                        \
  for (::logging::impl::RateLimiter log_limited_to_rl{                     \
           logger,                                                         \
           []() -> ::logging::impl::RateLimitData& {                       \
             thread_local ::logging::impl::RateLimitData rl_data;          \
             return rl_data;                                               \
           }(),                                                            \
           (lvl)};                                                         \
       log_limited_to_rl.ShouldLog(); log_limited_to_rl.SetShouldNotLog()) \
  LOG_TO((logger), log_limited_to_rl.GetLevel()) << log_limited_to_rl

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger. Ignores log messages that occur too often.
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED(lvl) LOG_LIMITED_TO(::logging::DefaultLogger(), lvl)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_TRACE() LOG_LIMITED(::logging::Level::kTrace)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_DEBUG() LOG_LIMITED(::logging::Level::kDebug)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_INFO() LOG_LIMITED(::logging::Level::kInfo)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_WARNING() LOG_LIMITED(::logging::Level::kWarning)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_ERROR() LOG_LIMITED(::logging::Level::kError)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_CRITICAL() LOG_LIMITED(::logging::Level::kCritical)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_TRACE_TO(logger) \
  LOG_LIMITED_TO(logger, ::logging::Level::kTrace)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_DEBUG_TO(logger) \
  LOG_LIMITED_TO(logger, ::logging::Level::kDebug)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_INFO_TO(logger) \
  LOG_LIMITED_TO(logger, ::logging::Level::kInfo)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_WARNING_TO(logger) \
  LOG_LIMITED_TO(logger, ::logging::Level::kWarning)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_ERROR_TO(logger) \
  LOG_LIMITED_TO(logger, ::logging::Level::kError)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_LIMITED_CRITICAL_TO(logger) \
  LOG_LIMITED_TO(logger, ::logging::Level::kCritical)
