#include <userver/server/congestion_control/limiter.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::congestion_control {

void Limiter::SetLimit(
    const USERVER_NAMESPACE::congestion_control::Limit& new_limit) {
  const auto limit = new_limit.load_limit;

  const auto lock = limitees_.Lock();
  for (const auto& limitee : *lock) {
    limitee->SetLimit(limit);
  }
}

void Limiter::RegisterLimitee(Limitee& limitee) {
  auto lock = limitees_.Lock();
  lock->emplace_back(&limitee);
}

}  // namespace server::congestion_control

USERVER_NAMESPACE_END
