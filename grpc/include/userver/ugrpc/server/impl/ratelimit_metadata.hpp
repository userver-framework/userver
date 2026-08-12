#pragma once

#include <grpcpp/server_context.h>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::impl {

void AddRatelimitMetadata(grpc::ServerContextBase& server_context);

}  // namespace ugrpc::server::impl

USERVER_NAMESPACE_END
