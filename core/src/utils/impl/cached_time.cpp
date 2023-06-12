#include <userver/utils/impl/cached_time.hpp>

#include <atomic>

#include <userver/utils/datetime.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::impl {

namespace {

std::atomic<SystemTimePoint> system_clock_now{{}};
std::atomic<SteadyTimePoint> steady_clock_now{{}};

}  // namespace

void UpdateGlobalTime() {
  // 'relaxed' reason: it is the user's job to ensure that their
  // UpdateGlobalTime call is ordered-before their GetGlobalTime calls.
  system_clock_now.store(utils::datetime::Now(), std::memory_order_relaxed);
  steady_clock_now.store(utils::datetime::SteadyNow(),
                         std::memory_order_relaxed);
}

std::tuple<SystemTimePoint, SteadyTimePoint> GetGlobalTime() {
  return {system_clock_now.load(std::memory_order_relaxed),
          steady_clock_now.load(std::memory_order_relaxed)};
}

}  // namespace utils::impl

USERVER_NAMESPACE_END
