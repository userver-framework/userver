#include <userver/utils/rand.hpp>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

void Rand(benchmark::State& state) {
  utils::Rand();  // initialize RNG

  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::Rand());
  }
}
BENCHMARK(Rand);

void RandRange(benchmark::State& state) {
  utils::Rand();  // initialize RNG

  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::RandRange(42));
  }
}
BENCHMARK(RandRange);

void WeakRandRange(benchmark::State& state) {
  utils::Rand();  // initialize RNG

  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::WeakRandRange(42));
  }
}
BENCHMARK(WeakRandRange);

USERVER_NAMESPACE_END
