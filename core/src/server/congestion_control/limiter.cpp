#include <server/congestion_control/limiter.hpp>

namespace server::congestion_control {

Limiter::Limiter(Server& server) : server_(server) {}

void Limiter::SetLimit(const ::congestion_control::Limit& new_limit) {
  server_.SetRpsRatelimit(new_limit.load_limit);
}

}  // namespace server::congestion_control
