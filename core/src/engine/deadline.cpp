#include <userver/engine/deadline.hpp>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

bool Deadline::IsReached() const noexcept {
  if (!IsReachable()) return false;
  if (value_ == kPassed) return true;

  return value_ <= TimePoint::clock::now();
}

Deadline::Duration Deadline::TimeLeft() const noexcept {
  UASSERT(IsReachable());
  if (value_ == kPassed) return Duration::zero();
  return value_ - TimePoint::clock::now();
}

}  // namespace engine

USERVER_NAMESPACE_END
