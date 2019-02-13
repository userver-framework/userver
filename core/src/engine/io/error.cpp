#include <engine/io/error.hpp>

#include <string>

#include <utils/strerror.hpp>

namespace engine {
namespace io {

IoError::IoError() : std::runtime_error("generic I/O error") {}

IoTimeout::IoTimeout() : IoTimeout(0) {}

IoTimeout::IoTimeout(size_t bytes_transferred)
    : IoError("I/O operation timed out"),
      bytes_transferred_(bytes_transferred) {}

IoCancelled::IoCancelled(const std::string& context)
    : IoError("Operation cancelled: " + context) {}

IoSystemError::IoSystemError(std::string message, int err_value)
    : IoSystemError(std::move(message),
                    std::error_code(err_value, std::system_category())) {}

IoSystemError::IoSystemError(std::string message, std::error_code code)
    : IoError(std::move(message) + ": " + code.message()),
      code_(std::move(code)) {}

}  // namespace io
}  // namespace engine
