#pragma once

#include <exception>

#include <grpcpp/support/status.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief Base class for userver-internal rpc errors
class BaseInternalRpcError : public std::exception {};

/// @brief Middleware interruption
class MiddlewareRpcInterruptionError : public BaseInternalRpcError {
public:
    explicit MiddlewareRpcInterruptionError(grpc::Status&& status) : status_(std::move(status)) {}

    grpc::Status&& ExtractStatus() { return std::move(status_); }

private:
    grpc::Status status_{};
};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
