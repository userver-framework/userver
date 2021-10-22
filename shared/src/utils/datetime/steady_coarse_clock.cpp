#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <ctime>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

#if defined(CLOCK_MONOTONIC_COARSE)
constexpr auto kCoarseClockNativeFlag = CLOCK_MONOTONIC_COARSE;
#else
constexpr auto kCoarseClockNativeFlag = CLOCK_MONOTONIC;
#endif

SteadyCoarseClock::time_point SteadyCoarseClock::now() noexcept {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  ::timespec tp;
  const auto res = ::clock_gettime(kCoarseClockNativeFlag, &tp);
  UASSERT_MSG(res == 0, "Must always succeed");
  return time_point(std::chrono::seconds(tp.tv_sec) +
                    std::chrono::nanoseconds(tp.tv_nsec));
}

SteadyCoarseClock::duration SteadyCoarseClock::resolution() noexcept {
  static const duration cached_resolution = []() {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ::timespec tp;
    const auto res = ::clock_getres(kCoarseClockNativeFlag, &tp);
    UASSERT_MSG(res == 0, "Must always succeed");
    return std::chrono::seconds(tp.tv_sec) +
           std::chrono::nanoseconds(tp.tv_nsec);
  }();

  return cached_resolution;
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
