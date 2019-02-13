#pragma once

#include <stdexcept>
#include <system_error>

namespace engine {
namespace io {

class IoError : public std::runtime_error {
 public:
  IoError();
  using std::runtime_error::runtime_error;
};

class IoTimeout : public IoError {
 public:
  IoTimeout();
  explicit IoTimeout(size_t bytes_transferred);

  size_t BytesTransferred() const { return bytes_transferred_; }

 private:
  size_t bytes_transferred_;
};

class IoCancelled : public IoError {
 public:
  explicit IoCancelled(const std::string& context);
};

class IoSystemError : public IoError {
 public:
  IoSystemError(std::string message, int err_value);
  IoSystemError(std::string message, std::error_code code);

  const std::error_code& Code() const { return code_; }

 private:
  std::error_code code_;
};

}  // namespace io
}  // namespace engine
