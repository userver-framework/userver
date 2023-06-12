#include <userver/engine/deadline.hpp>

#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime/steady_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

using CoarseClock = utils::datetime::SteadyCoarseClock;

bool Deadline::IsReached() const noexcept {
  if (!IsReachable()) return false;
  if (value_ == kPassed) return true;

  return value_ <= TimePoint::clock::now();
}

bool Deadline::IsSurelyReachedApprox() const noexcept {
  if (!IsReachable()) return false;
  if (value_ == kPassed) return true;

  return value_.time_since_epoch() <=
         CoarseClock::now().time_since_epoch() - CoarseClock::resolution();
}

Deadline::Duration Deadline::TimeLeft() const noexcept {
  UASSERT(IsReachable());
  if (value_ == kPassed) return Duration::zero();
  return value_ - TimePoint::clock::now();
}

Deadline::Duration Deadline::TimeLeftApprox() const noexcept {
  UASSERT(IsReachable());
  if (value_ == kPassed) return Duration::min();

  return value_.time_since_epoch() - CoarseClock::now().time_since_epoch();
}

void Deadline::OnDurationOverflow(
    std::chrono::duration<double> incoming_duration) {
  LOG_TRACE() << "Adding duration " << incoming_duration.count()
              << "s would have overflown deadline, so we replace it with "
              << "unreachable deadline" << logging::LogExtra::Stacktrace();
}

}  // namespace engine

USERVER_NAMESPACE_END
