#include <userver/utils/datetime/wall_coarse_clock.hpp>

#include <chrono>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

void wall_clock_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(std::chrono::system_clock::now());
  }
}
BENCHMARK(wall_clock_benchmark);

void wall_coarse_clock_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::datetime::WallCoarseClock::now());
  }
}
BENCHMARK(wall_coarse_clock_benchmark);

USERVER_NAMESPACE_END
