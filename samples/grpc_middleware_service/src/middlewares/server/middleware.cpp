#include "middleware.hpp"

#include <middlewares/auth.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component.hpp>

namespace sample::grpc::auth::server {

/// [gRPC middleware sample - Middleware implementation]
Middleware::Middleware() = default;

void Middleware::Handle(ugrpc::server::MiddlewareCallContext& context) const {
  const auto& metadata = context.GetCall().GetContext().client_metadata();

  auto it = metadata.find(kKey);

  if (it == metadata.cend() || it->second != kCredentials) {
    auto& rpc = context.GetCall();
    rpc.FinishWithError(::grpc::Status{::grpc::StatusCode::PERMISSION_DENIED,
                                       "Invalid credentials"});
    LOG_ERROR() << "Invalid credentials";
    return;
  }

  context.Next();
}
/// [gRPC middleware sample - Middleware implementation]

}  // namespace sample::grpc::auth::server
