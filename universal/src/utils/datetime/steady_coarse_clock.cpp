#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <utils/datetime/coarse_clock_gettime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

constexpr auto kClockFlag = kCoarseSteadyClockNativeFlag;

SteadyCoarseClock::time_point SteadyCoarseClock::now() noexcept {
  return CoarseNow<time_point, kClockFlag>();
}

SteadyCoarseClock::duration SteadyCoarseClock::resolution() noexcept {
  return CoarseResolution<duration, kClockFlag>();
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
