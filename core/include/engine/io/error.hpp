#pragma once

#include <stdexcept>
#include <system_error>

#include <utils/traceful_exception.hpp>

namespace engine::io {

/// Generic I/O error.
class IoError : public utils::TracefulException {
 public:
  IoError();
  using utils::TracefulException::TracefulException;
};

/// I/O timeout.
class IoTimeout : public IoError {
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
class IoCancelled : public IoError {
 public:
  IoCancelled();
};

/// Operating system I/O error.
class IoSystemError : public IoError {
 public:
  explicit IoSystemError(int err_value);
  explicit IoSystemError(std::error_code code);

  /// Operating system error code.
  const std::error_code& Code() const { return code_; }

 private:
  std::error_code code_;
};

}  // namespace engine::io
