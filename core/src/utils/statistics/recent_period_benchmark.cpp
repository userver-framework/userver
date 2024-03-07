#include <userver/utils/datetime/steady_coarse_clock.hpp>
#include <userver/utils/statistics/percentile.hpp>
#include <userver/utils/statistics/recentperiod.hpp>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

namespace {

using Percentile = utils::statistics::Percentile<2048>;

using DefaultClock = utils::datetime::SteadyClock;
using CoarseClock = utils::datetime::SteadyCoarseClock;

template <typename Clock>
using RecentPeriod =
    utils::statistics::RecentPeriod<Percentile, Percentile, Clock>;

RecentPeriod<DefaultClock> gRecentPeriodDefaultClock{};
RecentPeriod<CoarseClock> gRecentPeriodCoarseClock{};

template <typename Clock>
auto& GetRecentPeriod() {
  if constexpr (std::is_same_v<Clock, DefaultClock>) {
    return gRecentPeriodDefaultClock;
  } else if constexpr (std::is_same_v<Clock, CoarseClock>) {
    return gRecentPeriodCoarseClock;
  } else {
    static_assert(!sizeof(Clock));
  }
}

}  // namespace

// This is very close to how postgres driver accounts its stats
template <typename Clock>
void RecentPeriodOfPercentilesAccountBenchmark(benchmark::State& state) {
  auto& rp = GetRecentPeriod<Clock>();

  for ([[maybe_unused]] auto _ : state) {
    for (std::size_t i = 0; i < 10; ++i) {
      rp.GetCurrentCounter().Account(i);
    }
  }
}
BENCHMARK_TEMPLATE(RecentPeriodOfPercentilesAccountBenchmark, DefaultClock)
    ->ThreadRange(1, 16);
BENCHMARK_TEMPLATE(RecentPeriodOfPercentilesAccountBenchmark, CoarseClock)
    ->ThreadRange(1, 16);

USERVER_NAMESPACE_END
