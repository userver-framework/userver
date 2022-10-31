#include <userver/utils/datetime/wall_coarse_clock.hpp>

#include <utils/datetime/coarse_clock_gettime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

constexpr auto kClockFlag = kCoarseRealtimeClockNativeFlag;

WallCoarseClock::time_point WallCoarseClock::now() noexcept {
  return CoarseNow<time_point, kClockFlag>();
}

WallCoarseClock::duration WallCoarseClock::resolution() noexcept {
  return CoarseResolution<duration, kClockFlag>();
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
