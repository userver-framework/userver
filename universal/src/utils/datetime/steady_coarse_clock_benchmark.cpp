#include <userver/utils/datetime/steady_coarse_clock.hpp>

#include <chrono>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

void SteadyClockBenchmark(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(std::chrono::steady_clock::now());
    }
}
BENCHMARK(SteadyClockBenchmark);

void SteadyCoarseClockBenchmark(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(utils::datetime::SteadyCoarseClock::now());
    }
}
BENCHMARK(SteadyCoarseClockBenchmark);

USERVER_NAMESPACE_END
