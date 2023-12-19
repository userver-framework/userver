#pragma once

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::deadline_propagation {

/// @brief middleware for RPC handler logging settings
class Middleware final : public MiddlewareBase {
 public:
  explicit Middleware() = default;

  void Handle(MiddlewareCallContext& context) const override;
};

/// @cond
class MiddlewareFactory final : public MiddlewareFactoryBase {
 public:
  explicit MiddlewareFactory() = default;

  std::shared_ptr<const MiddlewareBase> GetMiddleware(
      std::string_view client_name) const override;
};
/// @endcond

}  // namespace ugrpc::client::middlewares::deadline_propagation

USERVER_NAMESPACE_END
