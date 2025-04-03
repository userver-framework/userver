#pragma once

#include <exception>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

/// @brief Base class for userver-internal rpc errors
class BaseInternalRpcError : public std::exception {};

/// @brief Middleware interruption
class MiddlewareRpcInterruptionError : public BaseInternalRpcError {};

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
