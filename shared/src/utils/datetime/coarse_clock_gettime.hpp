#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <ctime>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::datetime {

#if defined(CLOCK_MONOTONIC_COARSE)
constexpr auto kCoarseSteadyClockNativeFlag = CLOCK_MONOTONIC_COARSE;
#else
constexpr auto kCoarseSteadyClockNativeFlag = CLOCK_MONOTONIC;
#endif

#if defined(CLOCK_REALTIME_COARSE)
constexpr auto kCoarseRealtimeClockNativeFlag = CLOCK_REALTIME_COARSE;
#else
constexpr auto kCoarseRealtimeClockNativeFlag = CLOCK_REALTIME;
#endif

template <typename TimePoint, int Flag>
TimePoint CoarseNow() noexcept {
  static_assert(Flag == kCoarseSteadyClockNativeFlag ||
                Flag == kCoarseRealtimeClockNativeFlag);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
  ::timespec tp;
  const auto res = ::clock_gettime(static_cast<clockid_t>(Flag), &tp);
  UASSERT_MSG(res == 0, "Must always succeed");
  return TimePoint{std::chrono::duration_cast<typename TimePoint::duration>(
      std::chrono::seconds(tp.tv_sec) + std::chrono::nanoseconds(tp.tv_nsec))};
}

template <typename Duration, int Flag>
Duration CoarseResolution() noexcept {
  static_assert(Flag == kCoarseSteadyClockNativeFlag ||
                Flag == kCoarseRealtimeClockNativeFlag);

  static const Duration cached_resolution = []() {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    ::timespec tp;
    const auto res = ::clock_getres(static_cast<clockid_t>(Flag), &tp);
    UASSERT_MSG(res == 0, "Must always succeed");
    return std::chrono::duration_cast<Duration>(
        std::chrono::seconds(tp.tv_sec) + std::chrono::nanoseconds(tp.tv_nsec));
  }();

  return cached_resolution;
}

}  // namespace utils::datetime

USERVER_NAMESPACE_END
