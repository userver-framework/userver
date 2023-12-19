#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

namespace sample::grpc::auth::client {

/// [gRPC middleware sample - Middleware component declaration]
class Component final : public ugrpc::client::MiddlewareComponentBase {
 public:
  static constexpr const char* kName = "grpc-auth-client";

  Component(const components::ComponentConfig& config,
            const components::ComponentContext& context);

  std::shared_ptr<const ugrpc::client::MiddlewareFactoryBase>
  GetMiddlewareFactory() override;

 private:
  std::shared_ptr<ugrpc::client::MiddlewareFactoryBase> factory_;
};
/// [gRPC middleware sample - Middleware component declaration]

}  // namespace sample::grpc::auth::client
