#pragma once

/// @file userver/utils/traceful_exception.hpp
/// @brief @copybrief utils::TracefulException

#include <exception>
#include <string>
#include <type_traits>

#include <fmt/format.h>
#include <boost/stacktrace/stacktrace_fwd.hpp>

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Base class implementing backtrace storage and message builder,
/// published only for documentation purposes, please inherit from
/// utils::TracefulException instead.
class TracefulExceptionBase {
 public:
  static constexpr size_t kInlineBufferSize = 100;
  using MemoryBuffer = fmt::basic_memory_buffer<char, kInlineBufferSize>;

  virtual ~TracefulExceptionBase();
  TracefulExceptionBase(TracefulExceptionBase&&) noexcept;

  const MemoryBuffer& MessageBuffer() const noexcept;
  const boost::stacktrace::basic_stacktrace<>& Trace() const noexcept;

  /// Stream-like interface for message extension
  template <typename Exception, typename T>
  friend typename std::enable_if<
      std::is_base_of<TracefulExceptionBase,
                      typename std::remove_reference<Exception>::type>::value,
      Exception&&>::type
  operator<<(Exception&& ex, const T& data) {
    fmt::format_to(ex.GetMessageBuffer(), "{}", data);
    ex.EnsureNullTerminated();
    return std::forward<Exception>(ex);
  }

 protected:
  /// @cond
  /// Default constructor, internal use only
  TracefulExceptionBase();

  /// Initial message constructor, internal use only
  explicit TracefulExceptionBase(std::string_view what);

 private:
  void EnsureNullTerminated();

  MemoryBuffer& GetMessageBuffer();

  struct Impl;
  utils::FastPimpl<Impl, 160, 8> impl_;
};

/// @ingroup userver_base_classes
///
/// @brief Exception that remembers the backtrace at the point of its
/// construction
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
class TracefulException : public std::exception, public TracefulExceptionBase {
 public:
  TracefulException() = default;
  explicit TracefulException(std::string_view what)
      : TracefulExceptionBase(what) {}

  const char* what() const noexcept override;
};

namespace impl {

template <typename PlainException>
// NOLINTNEXTLINE(fuchsia-multiple-inheritance)
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

USERVER_NAMESPACE_END
