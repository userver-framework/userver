#include <userver/utils/datetime/wall_coarse_clock.hpp>

#include <chrono>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

void WallClockBenchmark(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(std::chrono::system_clock::now());
    }
}
BENCHMARK(WallClockBenchmark);

void WallCoarseClockBenchmark(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(utils::datetime::WallCoarseClock::now());
    }
}
BENCHMARK(WallCoarseClockBenchmark);

USERVER_NAMESPACE_END
