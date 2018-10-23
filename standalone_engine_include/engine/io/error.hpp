#pragma once

#include <stdexcept>

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

}  // namespace io
}  // namespace engine
