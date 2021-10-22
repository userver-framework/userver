#pragma once

/// @file userver/engine/io/exception.hpp
/// @brief I/O exceptions

#include <stdexcept>
#include <system_error>

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// Generic I/O error.
class IoException : public utils::TracefulException {
 public:
  IoException();
  using utils::TracefulException::TracefulException;
};

/// I/O timeout.
class IoTimeout : public IoException {
 public:
  IoTimeout();
  explicit IoTimeout(size_t bytes_transferred);

  /// Number of bytes transferred before timeout.
  size_t BytesTransferred() const { return bytes_transferred_; }

 private:
  size_t bytes_transferred_;
};

/// Task cancellation during I/O.
/// Context description is expected to be provided by user.
class IoCancelled : public IoException {
 public:
  IoCancelled();
};

/// Operating system I/O error.
class IoSystemError : public IoException {
 public:
  explicit IoSystemError(int err_value);
  explicit IoSystemError(std::error_code code);

  /// Operating system error code.
  const std::error_code& Code() const { return code_; }

 private:
  std::error_code code_;
};

}  // namespace engine::io

USERVER_NAMESPACE_END
