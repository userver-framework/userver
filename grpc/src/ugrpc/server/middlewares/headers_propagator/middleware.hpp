#pragma once

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::headers_propagator {

class Middleware final : public MiddlewareBase {
 public:
  explicit Middleware(std::vector<std::string> headers);

  void Handle(MiddlewareCallContext& context) const override;

 private:
  const std::vector<std::string> headers_;
};

}  // namespace ugrpc::server::middlewares::headers_propagator

USERVER_NAMESPACE_END
