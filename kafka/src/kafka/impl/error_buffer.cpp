#include <kafka/impl/error_buffer.hpp>

#include <cstring>

#include <fmt/format.h>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace kafka::impl {

void PrintErrorAndThrow(std::string_view failed_action, const ErrorBuffer& err_buf) {
    std::string_view reason{err_buf.data(), std::strlen(err_buf.data())};

    const auto full_error = fmt::format("Failed to {}: {}", failed_action, reason);
    LOG_ERROR() << full_error;
    throw std::runtime_error{full_error};
}

}  // namespace kafka::impl

USERVER_NAMESPACE_END
