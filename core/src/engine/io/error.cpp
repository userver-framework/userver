#include <engine/io/error.hpp>

#include <string>

#include <utils/strerror.hpp>

namespace engine {
namespace io {

IoError::IoError() : utils::TracefulException("Generic I/O error") {}

IoTimeout::IoTimeout() : IoTimeout(0) {}

IoTimeout::IoTimeout(size_t bytes_transferred)
    : IoError("I/O operation timed out"),
      bytes_transferred_(bytes_transferred) {}

IoCancelled::IoCancelled() : IoError("Operation cancelled: ") {}

IoSystemError::IoSystemError(int err_value)
    : IoSystemError(std::error_code(err_value, std::system_category())) {}

IoSystemError::IoSystemError(std::error_code code)
    : IoError(code.message() + ": "), code_(code) {}

}  // namespace io
}  // namespace engine
