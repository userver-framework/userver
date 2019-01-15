#pragma once

/// @file logging/log.hpp
/// @brief Logging helpers

#include <memory>
#include <ostream>

#include <boost/system/error_code.hpp>

#include <utils/encoding/hex.hpp>
#include <utils/encoding/tskv.hpp>

#include "level.hpp"
#include "log_extra.hpp"
#include "logger.hpp"

namespace logging {

/// Returns default logger
LoggerPtr DefaultLogger();

/// Atomically replaces default logger and returns the old one
LoggerPtr SetDefaultLogger(LoggerPtr);

/// Sets new log level for default logger
void SetDefaultLoggerLevel(Level);

/// Stream-like tskv-formatted log message builder
class LogHelper {
 public:
  /// @brief Constructs LogHelper
  /// @param level message log level
  /// @param path path of the source file that generated the message
  /// @param line line of the source file that generated the message
  /// @param func name of the function that generated the message
  LogHelper(Level level, const char* path, int line, const char* func);
  ~LogHelper();

  LogHelper(LogHelper&&) = delete;
  LogHelper(const LogHelper&) = delete;
  LogHelper& operator=(LogHelper&&) = delete;
  LogHelper& operator=(const LogHelper&) = delete;

  // All the logging goes through this function
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
    lh.Stream() << reinterpret_cast<unsigned long long>(value);
    return lh;
  }

  friend LogHelper& operator<<(LogHelper& lh, void* value) {
    lh.Stream() << reinterpret_cast<unsigned long long>(value);
    return lh;
  }

  friend LogHelper& operator<<(LogHelper& lh, boost::system::error_code ec) {
    lh.Stream() << ec.category().name() << ':' << ec.value();
    return lh;
  }

  template <typename T>
  friend
      typename std::enable_if<!utils::encoding::TypeNeedsEncodeTskv<T>::value,
                              LogHelper&>::type
      operator<<(LogHelper& lh, const T& value) {
    lh.Stream() << value;
    return lh;
  }

  template <typename T>
  friend typename std::enable_if<utils::encoding::TypeNeedsEncodeTskv<T>::value,
                                 LogHelper&>::type
  operator<<(LogHelper& lh, const T& value) {
    try {
      lh.StartTskvEncoding();
      lh.Stream() << value;
    } catch (...) {
      lh.EndTskvEncoding();
      throw;
    }

    lh.EndTskvEncoding();
    return lh;
  }

  friend LogHelper& operator<<(LogHelper& lh, LogExtra extra) {
    lh.extra_.Extend(std::move(extra));
    return lh;
  }

 private:
  void DoLog() noexcept;

  void AppendLogExtra();
  void LogTextKey();
  void LogModule(const char* path, int line, const char* func);
  void LogTaskIdAndCoroutineId();
  void LogSpan();

  std::ostream& Stream() noexcept;
  void StartTskvEncoding() noexcept;
  void EndTskvEncoding() noexcept;

  struct StreamImpl;
  static constexpr std::size_t kStreamImplSize = 936;
  utils::FastPimpl<StreamImpl, kStreamImplSize, 8, true> pimpl_;

  LogExtra extra_;
};

inline LogHelper& operator<<(LogHelper& lh, std::error_code ec) {
  lh << ec.category().name() << ":" << ec.value() << " (" << ec.message()
     << ")";
  return lh;
}

/// Forces flush of default logger message queue
void LogFlush();

}  // namespace logging

/// @brief Builds a stream and evaluates a message for the default logger
/// if lvl matches the verbosity, otherwise the message is not evaluated
/// @hideinitializer
#define LOG(lvl)                                                      \
  for (bool _need_log = ShouldLog(lvl); _need_log; _need_log = false) \
  ::logging::LogHelper(lvl, FILENAME, __LINE__, __func__)

#define LOG_TRACE() LOG(::logging::Level::kTrace)
#define LOG_DEBUG() LOG(::logging::Level::kDebug)
#define LOG_INFO() LOG(::logging::Level::kInfo)
#define LOG_WARNING() LOG(::logging::Level::kWarning)
#define LOG_ERROR() LOG(::logging::Level::kError)
#define LOG_CRITICAL() LOG(::logging::Level::kCritical)
