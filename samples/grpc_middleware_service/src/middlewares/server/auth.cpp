#include "auth.hpp"

#include <middlewares/auth.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component.hpp>

namespace samples::grpc::auth::server {

/// [Middleware implementation]
Middleware::Middleware() = default;

void Middleware::OnCallStart(ugrpc::server::MiddlewareCallContext& context) const {
    const auto& metadata = context.GetServerContext().client_metadata();

    auto it = metadata.find(kKey);

    if (it == metadata.cend() || it->second != kCredentials) {
        LOG_ERROR() << "Invalid credentials";
        return context.SetError(::grpc::Status{::grpc::StatusCode::PERMISSION_DENIED, "Invalid credentials"});
    }
}
/// [Middleware implementation]

}  // namespace samples::grpc::auth::server
