#pragma once

#include <grpcpp/support/status.h>

#include <userver/server/handlers/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

grpc::StatusCode CustomStatusToGrpc(
    USERVER_NAMESPACE::server::handlers::HandlerErrorCode code);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
