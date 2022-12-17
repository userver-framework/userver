#pragma once

/// @file userver/engine/io/exception.hpp
/// @brief I/O exceptions

#include <stdexcept>
#include <string_view>
#include <system_error>

#include <userver/utils/traceful_exception.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

/// Generic I/O error.
class IoException : public utils::TracefulException {
 public:
  IoException();

  explicit IoException(std::string_view message);
};

/// I/O interruption.
class IoInterrupted : public IoException {
 public:
  explicit IoInterrupted(std::string_view reason, size_t bytes_transferred);

  /// Number of bytes transferred before interruption.
  size_t BytesTransferred() const { return bytes_transferred_; }

 private:
  size_t bytes_transferred_;
};

/// I/O timeout.
class IoTimeout : public IoInterrupted {
 public:
  IoTimeout();
  explicit IoTimeout(size_t bytes_transferred);
};

/// Task cancellation during I/O.
/// Context description is expected to be provided by user.
class IoCancelled : public IoInterrupted {
 public:
  IoCancelled();
  explicit IoCancelled(size_t bytes_transferred);
};

/// Operating system I/O error.
class IoSystemError : public IoException {
 public:
  IoSystemError(int err_value, std::string_view reason);
  IoSystemError(std::error_code code, std::string_view reason);

  /// Operating system error code.
  const std::error_code& Code() const { return code_; }

 private:
  std::error_code code_;
};

/// TLS I/O error.
class TlsException : public IoException {
 public:
  using IoException::IoException;
  ~TlsException() override;
};

}  // namespace engine::io

USERVER_NAMESPACE_END
