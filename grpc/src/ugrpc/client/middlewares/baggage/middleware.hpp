#pragma once

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::baggage {

/// @brief middleware for gRPC client baggage
class Middleware final : public MiddlewareBase {
 public:
  void Handle(MiddlewareCallContext& context) const override;
};
///

class MiddlewareFactory final : public MiddlewareFactoryBase {
 public:
  std::shared_ptr<const MiddlewareBase> GetMiddleware(
      std::string_view client_name) const override;
};

}  // namespace ugrpc::client::middlewares::baggage

USERVER_NAMESPACE_END
