#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <chrono>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

void steady_clock_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(std::chrono::steady_clock::now());
  }
}
BENCHMARK(steady_clock_benchmark);

void steady_coarse_clock_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::datetime::SteadyCoarseClock::now());
  }
}
BENCHMARK(steady_coarse_clock_benchmark);

USERVER_NAMESPACE_END
