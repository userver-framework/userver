#include "middleware.hpp"

#include <middlewares/auth.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace sample::grpc::auth::client {

/// [gRPC middleware sample - Middleware implementation]
void ApplyCredentials(::grpc::ClientContext& context) { context.AddMetadata(kKey, kCredentials); }

Middleware::Middleware() = default;

Middleware::~Middleware() = default;

void Middleware::PreStartCall(ugrpc::client::MiddlewareCallContext& context) const {
    ApplyCredentials(context.GetContext());
}

/// [gRPC middleware sample - Middleware implementation]

}  // namespace sample::grpc::auth::client
