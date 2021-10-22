#pragma once

#include <userver/congestion_control/limiter.hpp>
#include <userver/server/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

class Limiter final : public USERVER_NAMESPACE::congestion_control::Limiter {
 public:
  explicit Limiter(Server& server);

  void SetLimit(
      const USERVER_NAMESPACE::congestion_control::Limit& new_limit) override;

 private:
  Server& server_;
};

}  // namespace server::congestion_control

USERVER_NAMESPACE_END
