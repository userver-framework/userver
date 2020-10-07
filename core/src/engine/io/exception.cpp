#include <engine/io/exception.hpp>

#include <string>

#include <utils/strerror.hpp>

namespace engine::io {

IoException::IoException() : utils::TracefulException("Generic I/O error") {}

IoTimeout::IoTimeout() : IoTimeout(0) {}

IoTimeout::IoTimeout(size_t bytes_transferred)
    : IoException("I/O operation timed out"),
      bytes_transferred_(bytes_transferred) {}

IoCancelled::IoCancelled() : IoException("Operation cancelled: ") {}

IoSystemError::IoSystemError(int err_value)
    : IoSystemError(std::error_code(err_value, std::system_category())) {}

IoSystemError::IoSystemError(std::error_code code)
    : IoException(code.message() + ": "), code_(code) {}

}  // namespace engine::io
