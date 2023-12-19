#pragma once

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::baggage {

class Middleware final : public MiddlewareBase {
 public:
  void Handle(MiddlewareCallContext& context) const override;
};

}  // namespace ugrpc::server::middlewares::baggage

USERVER_NAMESPACE_END
