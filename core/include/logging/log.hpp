#pragma once

/// @file logging/log.hpp
/// @brief Logging helpers

#include <chrono>
#include <exception>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>

#include <boost/system/error_code.hpp>

#include <utils/encoding/hex.hpp>
#include <utils/encoding/tskv.hpp>

#include "level.hpp"
#include "log_extra.hpp"
#include "logger.hpp"

namespace logging {

namespace impl {

struct Noop {};

}  // namespace impl

/// Returns default logger
LoggerPtr DefaultLogger();

/// Atomically replaces default logger and returns the old one
LoggerPtr SetDefaultLogger(LoggerPtr);

/// Sets new log level for default logger
void SetDefaultLoggerLevel(Level);

/// Returns log level for default logger
Level GetDefaultLoggerLevel();

/// Stream-like tskv-formatted log message builder.
///
/// Users can add LogHelper& operator<<(LogHelper&, ) overloads to use a faster
/// localeless logging, rather than outputting data through the ostream
/// operator.
class LogHelper final {
 public:
  enum class Mode { kDefault, kNoSpan };

  /// @brief Constructs LogHelper with span logging
  /// @param level message log level
  /// @param path path of the source file that generated the message
  /// @param line line of the source file that generated the message
  /// @param func name of the function that generated the message
  /// @param mode logging mode - with or without span
  LogHelper(LoggerPtr logger, Level level, const char* path, int line,
            const char* func, Mode mode = Mode::kDefault) noexcept;

  ~LogHelper();

  LogHelper(LogHelper&&) = delete;
  LogHelper(const LogHelper&) = delete;
  LogHelper& operator=(LogHelper&&) = delete;
  LogHelper& operator=(const LogHelper&) = delete;

  // Helper function that could be called on LogHelper&& to get LogHelper&.
  LogHelper& AsLvalue() noexcept { return *this; }

  template <typename T>
  LogHelper& operator<<(const T& value) {
    constexpr bool encode = utils::encoding::TypeNeedsEncodeTskv<T>::value;
    EncodingGuard guard{*this, encode ? Encode::kValue : Encode::kNone};

    constexpr bool is_char_like =
        std::is_constructible<std::string_view, T>::value ||
        std::is_same<char, T>::value || std::is_same<signed char, T>::value ||
        std::is_same<unsigned char, T>::value;

    if constexpr (is_char_like) {
      Put(value);
    } else if constexpr (std::is_floating_point<T>::value) {
      PutFloatingPoint(value);
    } else if constexpr (std::is_signed<T>::value) {
      PutSigned(value);
    } else if constexpr (std::is_unsigned<T>::value) {
      PutUnsigned(value);
    } else if constexpr (std::is_base_of<std::exception, T>::value) {
      PutException(value);
    } else {
      // No operator<< overload for LogHelper and type T. You could provide one,
      // to speed up logging of value.
      Stream() << value;
    }

    return *this;
  }

  LogHelper& operator<<(const LogExtra& extra) {
    extra_.Extend(extra);
    return *this;
  }

  LogHelper& operator<<(LogExtra&& extra) {
    extra_.Extend(std::move(extra));
    return *this;
  }

  /// Outputs value in a hex mode with the shortest representation, just like
  /// std::to_chars does.
  void PutHexShort(unsigned long long value);

  /// Outputs value in a hex mode with the fixed length representation.
  void PutHex(const void* value);

  /// @cond
  // For internal use only!
  operator impl::Noop() const noexcept { return {}; }
  /// @endcond

 private:
  enum class Encode { kNone, kValue, kKeyReplacePeriod };

  struct EncodingGuard {
    LogHelper& lh;

    EncodingGuard(LogHelper& lh, Encode mode) noexcept;
    ~EncodingGuard();
  };

  void DoLog() noexcept;

  void AppendLogExtra();
  void LogTextKey();
  void LogModule(const char* path, int line, const char* func);
  void LogIds();
  void LogSpan();

  void PutFloatingPoint(float value);
  void PutFloatingPoint(double value);
  void PutFloatingPoint(long double value);
  void PutUnsigned(unsigned long long value);
  void PutSigned(long long value);
  void Put(std::string_view value);
  void Put(char value);
  void PutException(const std::exception& ex);

  std::ostream& Stream();

  class Impl;
#ifdef _LIBCPP_VERSION
  static constexpr std::size_t kStreamImplSize = 856;
#else
  static constexpr std::size_t kStreamImplSize = 968;
#endif
  utils::FastPimpl<Impl, kStreamImplSize, 8, true> pimpl_;

  LogExtra extra_;
};

inline LogHelper& operator<<(LogHelper& lh, std::error_code ec) {
  lh << ec.category().name() << ':' << ec.value() << " (" << ec.message()
     << ')';
  return lh;
}

template <typename T>
inline LogHelper& operator<<(LogHelper& lh, const std::atomic<T>& value) {
  return lh << value.load();
}

inline LogHelper& operator<<(LogHelper& lh, const void* value) {
  lh.PutHex(value);
  return lh;
}

inline LogHelper& operator<<(LogHelper& lh, void* value) {
  lh.PutHex(value);
  return lh;
}

inline LogHelper& operator<<(LogHelper& lh, boost::system::error_code ec) {
  lh << ec.category().name() << ':' << ec.value();
  return lh;
}

template <typename T>
inline LogHelper& operator<<(LogHelper& lh, const std::optional<T>& value) {
  if (value)
    lh << *value;
  else
    lh << "(none)";
  return lh;
}

template <class Result, class... Args>
inline LogHelper& operator<<(LogHelper& lh, Result (*)(Args...)) {
  static_assert(!sizeof(Result),
                "Outputing functions or std::ostream formatters is forbidden");
  return lh;
}

LogHelper& operator<<(LogHelper& lh, std::chrono::system_clock::time_point tp);

/// Forces flush of default logger message queue
void LogFlush();

}  // namespace logging

/// @brief Builds a stream and evaluates a message for the logger.
/// @hideinitializer
#define DO_LOG_TO(logger, lvl) \
  ::logging::LogHelper(logger, lvl, __FILE__, __LINE__, __func__).AsLvalue()

// static_cast<int> below are workarounds for clangs -Wtautological-compare

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the logger.
/// @hideinitializer
#define LOG_TO(logger, lvl)                                              \
  __builtin_expect(                                                      \
      !::logging::ShouldLog(lvl),                                        \
      static_cast<int>(lvl) < static_cast<int>(::logging::Level::kInfo)) \
      ? ::logging::impl::Noop{}                                          \
      : DO_LOG_TO(logger, lvl)

/// @brief If lvl matches the verbosity then builds a stream and evaluates a
/// message for the default logger.
#define LOG(lvl) LOG_TO(::logging::DefaultLogger(), lvl)

#define LOG_TRACE() LOG(::logging::Level::kTrace)
#define LOG_DEBUG() LOG(::logging::Level::kDebug)
#define LOG_INFO() LOG(::logging::Level::kInfo)
#define LOG_WARNING() LOG(::logging::Level::kWarning)
#define LOG_ERROR() LOG(::logging::Level::kError)
#define LOG_CRITICAL() LOG(::logging::Level::kCritical)

#define LOG_TRACE_TO(logger) LOG_TO(logger, ::logging::Level::kTrace)
#define LOG_DEBUG_TO(logger) LOG_TO(logger, ::logging::Level::kDebug)
#define LOG_INFO_TO(logger) LOG_TO(logger, ::logging::Level::kInfo)
#define LOG_WARNING_TO(logger) LOG_TO(logger, ::logging::Level::kWarning)
#define LOG_ERROR_TO(logger) LOG_TO(logger, ::logging::Level::kError)
#define LOG_CRITICAL_TO(logger) LOG_TO(logger, ::logging::Level::kCritical)
