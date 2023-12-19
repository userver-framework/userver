#include "middleware.hpp"

#include <middlewares/auth.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace sample::grpc::auth::client {

/// [gRPC middleware sample - Middleware and MiddlewareFactory implementation]
void ApplyCredentials(::grpc::ClientContext& context) {
  context.AddMetadata(kKey, kCredentials);
}

Middleware::Middleware() = default;

Middleware::~Middleware() = default;

void Middleware::Handle(ugrpc::client::MiddlewareCallContext& context) const {
  ApplyCredentials(context.GetCall().GetContext());
  context.Next();
}

MiddlewareFactory::MiddlewareFactory(const components::ComponentContext&) {}

MiddlewareFactory::~MiddlewareFactory() = default;

std::shared_ptr<const Middleware::MiddlewareBase>
MiddlewareFactory::GetMiddleware(std::string_view) const {
  return std::make_shared<Middleware>();
}
/// [gRPC middleware sample - Middleware and MiddlewareFactory implementation]

}  // namespace sample::grpc::auth::client
