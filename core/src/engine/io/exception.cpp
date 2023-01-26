#include <userver/engine/io/exception.hpp>

#include <fmt/compile.h>
#include <fmt/format.h>

#include <utils/strerror.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::io {

IoException::IoException() : IoException("Generic I/O error") {}

IoException::IoException(std::string_view message)
    : utils::TracefulException(
          utils::TracefulException::TraceMode::kIfLoggingIsEnabled) {
  *this << message;
}

IoInterrupted::IoInterrupted(std::string_view reason, size_t bytes_transferred)
    : IoException(reason), bytes_transferred_(bytes_transferred) {}

IoTimeout::IoTimeout() : IoTimeout(0) {}

IoTimeout::IoTimeout(size_t bytes_transferred)
    : IoInterrupted("I/O operation timed out: ", bytes_transferred) {}

IoCancelled::IoCancelled() : IoCancelled(0) {}

IoCancelled::IoCancelled(size_t bytes_transferred)
    : IoInterrupted("I/O operation cancelled: ", bytes_transferred) {}

IoSystemError::IoSystemError(int err_value, std::string_view reason)
    : IoSystemError(std::error_code(err_value, std::system_category()),
                    reason) {}

IoSystemError::IoSystemError(std::error_code code, std::string_view reason)
    : IoException(fmt::format(FMT_COMPILE("{}: {}"), reason, code.message())),
      code_(code) {}

TlsException::~TlsException() = default;

}  // namespace engine::io

USERVER_NAMESPACE_END
