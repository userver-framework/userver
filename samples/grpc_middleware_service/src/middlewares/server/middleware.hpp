#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

namespace sample::grpc::auth::server {

/// [gRPC middleware sample - Middleware declaration]
class Middleware final : public ugrpc::server::MiddlewareBase {
 public:
  explicit Middleware();

  void Handle(ugrpc::server::MiddlewareCallContext& context) const override;
};
/// [gRPC middleware sample - Middleware declaration]

}  // namespace sample::grpc::auth::server
