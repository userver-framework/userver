#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

namespace sample::grpc::auth::client {

/// [gRPC middleware sample - Middleware and MiddlewareFactory declaration]
class Middleware final : public ugrpc::client::MiddlewareBase {
 public:
  explicit Middleware();

  ~Middleware() override;

  void Handle(ugrpc::client::MiddlewareCallContext& context) const override;
};

class MiddlewareFactory final : public ugrpc::client::MiddlewareFactoryBase {
 public:
  explicit MiddlewareFactory(const components::ComponentContext& context);

  ~MiddlewareFactory() override;

  std::shared_ptr<const ugrpc::client::MiddlewareBase> GetMiddleware(
      std::string_view) const override;
};
/// [gRPC middleware sample - Middleware and MiddlewareFactory declaration]

}  // namespace sample::grpc::auth::client
