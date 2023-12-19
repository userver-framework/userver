#pragma once

#include <userver/server/congestion_control/limiter.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::congestion_control {

class Middleware final
    : public MiddlewareBase,
      public USERVER_NAMESPACE::server::congestion_control::Limitee {
 public:
  void Handle(MiddlewareCallContext& context) const override;

  void SetLimit(std::optional<size_t> new_limit) override;

 private:
  mutable utils::TokenBucket rate_limit_{utils::TokenBucket::MakeUnbounded()};
};

}  // namespace ugrpc::server::middlewares::congestion_control

USERVER_NAMESPACE_END
