#pragma once

#include <userver/congestion_control/limiter.hpp>
#include <userver/server/server.hpp>

namespace server::congestion_control {

class Limiter final : public ::congestion_control::Limiter {
 public:
  explicit Limiter(Server& server);

  void SetLimit(const ::congestion_control::Limit& new_limit) override;

 private:
  Server& server_;
};

}  // namespace server::congestion_control
