#pragma once

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::deadline_propagation {

class Middleware final : public MiddlewareBase {
 public:
  void Handle(MiddlewareCallContext& context) const override;
};

}  // namespace ugrpc::server::middlewares::deadline_propagation

USERVER_NAMESPACE_END
