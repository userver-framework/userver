#include <userver/cache/expirable_lru_cache.hpp>

USERVER_NAMESPACE_BEGIN

namespace cache::impl {

namespace {

std::atomic<SystemTimePoint> system_clock_now{{}};
std::atomic<SteadyTimePoint> steady_clock_now{{}};

}  // namespace

void UpdateGlobalTime() {
  system_clock_now.store(utils::datetime::Now(), std::memory_order_relaxed);
  steady_clock_now.store(utils::datetime::SteadyNow(),
                         std::memory_order_relaxed);
}

std::tuple<SystemTimePoint, SteadyTimePoint> GetGlobalTime() {
  return {system_clock_now.load(std::memory_order_relaxed),
          steady_clock_now.load(std::memory_order_relaxed)};
}

}  // namespace cache::impl

USERVER_NAMESPACE_END
