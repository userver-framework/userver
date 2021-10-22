#include <server/congestion_control/limiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

Limiter::Limiter(Server& server) : server_(server) {}

void Limiter::SetLimit(
    const USERVER_NAMESPACE::congestion_control::Limit& new_limit) {
  server_.SetRpsRatelimit(new_limit.load_limit);
}

}  // namespace server::congestion_control

USERVER_NAMESPACE_END
