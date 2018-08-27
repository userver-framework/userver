#pragma once

/// @file logging/log.hpp
/// @brief Logging helpers

#include <memory>

#include <spdlog/common.h>

#include <yandex/taxi/userver/utils/encoding/hex.hpp>
#include <yandex/taxi/userver/utils/encoding/tskv.hpp>

#include "level.hpp"
#include "log_extra.hpp"
#include "logger.hpp"

namespace logging {

/// Returns default logger
LoggerPtr DefaultLogger();

/// Atomically replaces default logger and returns the old one
LoggerPtr SetDefaultLogger(LoggerPtr);

/// Stream-like tskv-formatted log message builder
class LogHelper {
 public:
  /// @brief Constructs LogHelper
  /// @param level message log level
  /// @param path path of the source file that generated the message
  /// @param line line of the source file that generated the message
  /// @param func name of the function that generated the message
  LogHelper(Level level, const char* path, int line, const char* func);
  ~LogHelper() noexcept(false) { DoLog(); }

  template <typename T>
  friend LogHelper&& operator<<(LogHelper&& lh, const T& value) {
    lh << value;
    return std::move(lh);
  }

  template <typename T>
  friend LogHelper& operator<<(LogHelper& lh, const std::atomic<T>& value) {
    return lh << value.load();
  }

  friend LogHelper& operator<<(LogHelper& lh, const void* value) {
    lh.log_msg_.raw << reinterpret_cast<unsigned long long>(value);
    return lh;
  }

  friend LogHelper& operator<<(LogHelper& lh, void* value) {
    lh.log_msg_.raw << reinterpret_cast<unsigned long long>(value);
    return lh;
  }

  template <typename T>
  friend
      typename std::enable_if<!utils::encoding::TypeNeedsEncodeTskv<T>::value,
                              LogHelper&>::type
      operator<<(LogHelper& lh, const T& value) {
    lh.log_msg_.raw << value;
    return lh;
  }

  template <typename T>
  friend typename std::enable_if<utils::encoding::TypeNeedsEncodeTskv<T>::value,
                                 LogHelper&>::type
  operator<<(LogHelper& lh, const T& value) {
    utils::encoding::EncodeTskv(lh.log_msg_.raw, value,
                                utils::encoding::EncodeTskvMode::kValue);
    return lh;
  }

  friend LogHelper& operator<<(LogHelper& lh, LogExtra extra) {
    lh.extra_.Extend(std::move(extra));
    return lh;
  }

 private:
  void DoLog();

  void AppendLogExtra();
  void LogTextKey();
  void LogModule(const char* path, int line, const char* func);

  spdlog::details::log_msg log_msg_;
  LogExtra extra_;
};

inline LogHelper& operator<<(LogHelper& lh, std::error_code ec) {
  lh << ec.category().name() << ":" << ec.value() << " (" << ec.message()
     << ")";
  return lh;
}

LogHelper& operator<<(LogHelper& lh, std::thread::id id);

}  // namespace logging

/// @brief Builds a stream and evaluates a message for the default logger
/// if lvl matches the verbosity, otherwise the message is not evaluated
/// @hideinitializer
#define LOG(lvl)                                                \
  for (bool _need_log = ::logging::DefaultLogger()->should_log( \
           ::logging::impl::ToSpdlogLevel(lvl));                \
       _need_log; _need_log = false)                            \
  ::logging::LogHelper(lvl, FILENAME, __LINE__, __func__)

#define LOG_TRACE() LOG(::logging::Level::kTrace)
#define LOG_DEBUG() LOG(::logging::Level::kDebug)
#define LOG_INFO() LOG(::logging::Level::kInfo)
#define LOG_WARNING() LOG(::logging::Level::kWarning)
#define LOG_ERROR() LOG(::logging::Level::kError)
#define LOG_CRITICAL() LOG(::logging::Level::kCritical)

/// Forces flush of default logger message queue
#define LOG_FLUSH() ::logging::DefaultLogger()->flush()
