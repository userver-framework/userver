#pragma once

/// @file utils/traceful_exception.hpp
/// @brief @copybrief utils::TracefulException

#include <exception>
#include <string>
#include <type_traits>

#include <boost/lexical_cast.hpp>
#include <boost/stacktrace/stacktrace.hpp>

namespace utils {
namespace impl {
template <typename T>
class TraceAttachedException;
}  // namespace impl

/// Exception that remembers the backtrace at the point of its construction
class TracefulException;

/// Base class implementing backtrace storage and message builder, published
/// only for documentation purposes, please inherit from TracefulException
/// instead
class TracefulExceptionBase {
 public:
  virtual ~TracefulExceptionBase() {}

  const std::string& Message() const noexcept { return message_; }
  const boost::stacktrace::stacktrace& Trace() const noexcept {
    return stacktrace_;
  }

  /// Stream-like interface for message extension
  // TODO: TAXICOMMON-474
  template <typename Exception, typename T>
  friend typename std::enable_if<
      std::is_base_of<TracefulExceptionBase,
                      typename std::remove_reference<Exception>::type>::value,
      Exception&&>::type
  operator<<(Exception&& ex, const T& data) {
    ex.message_ += boost::lexical_cast<std::string>(data);
    return std::forward<Exception>(ex);
  }

 protected:
  /// @cond
  /// Default constructor, internal use only
  TracefulExceptionBase() = default;

  /// Initial message constructor, internal use only
  explicit TracefulExceptionBase(std::string what)
      : message_(std::move(what)) {}
  /// @endcond

 private:
  std::string message_;
  boost::stacktrace::stacktrace stacktrace_;
};

class TracefulException : public std::exception, public TracefulExceptionBase {
 public:
  TracefulException() = default;
  TracefulException(std::string what)
      : TracefulExceptionBase(std::move(what)) {}

  const char* what() const noexcept override;

 private:
#ifndef NDEBUG
  mutable std::string what_buffer_;
#endif  // NDEBUG
};

namespace impl {

template <typename PlainException>
class ExceptionWithAttachedTrace final : public PlainException,
                                         public TracefulExceptionBase {
 public:
  explicit ExceptionWithAttachedTrace(const PlainException& ex)
      : PlainException(ex), TracefulExceptionBase(ex.what()) {}
};

template <typename Exception>
std::enable_if_t<std::is_base_of<std::exception, Exception>::value,
                 ExceptionWithAttachedTrace<Exception>>
AttachTraceToException(const Exception& ex) {
  static_assert(!std::is_base_of<TracefulExceptionBase, Exception>::value,
                "This exception already contains trace");
  return ExceptionWithAttachedTrace<Exception>(ex);
}

}  // namespace impl
}  // namespace utils
