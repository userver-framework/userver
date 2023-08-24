#include <userver/ugrpc/server/exceptions.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

BaseError::BaseError(std::string message) : message_(std::move(message)) {}

const char* BaseError::what() const noexcept { return message_.c_str(); }

RpcError::RpcError(std::string_view call_name, std::string_view additional_info)
    : BaseError(fmt::format("'{}' failed: {}", call_name, additional_info)) {}

RpcInterruptedError::RpcInterruptedError(std::string_view call_name,
                                         std::string_view stage)
    : RpcError(call_name, fmt::format("connection error at {}", stage)) {}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
