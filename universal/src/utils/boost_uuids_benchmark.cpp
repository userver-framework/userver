#include <benchmark/benchmark.h>

#include <userver/utils/boost_uuid4.hpp>
#include <userver/utils/boost_uuid7.hpp>

USERVER_NAMESPACE_BEGIN

template <typename UuidGenerator>
void GenerateUuid(UuidGenerator generator, benchmark::State& state) {
  for ([[maybe_unused]] auto _ : state) {
    for (int i = 0; i < state.range(0); ++i) {
      benchmark::DoNotOptimize(generator());
      benchmark::ClobberMemory();
    }
  }
}

void GenerateUuidV4(benchmark::State& state) {
  GenerateUuid(&utils::generators::GenerateBoostUuid, state);
}

void GenerateUuidV7(benchmark::State& state) {
  GenerateUuid(&utils::generators::GenerateBoostUuidV7, state);
}

BENCHMARK(GenerateUuidV4)->RangeMultiplier(2)->Range(1, 1 << 12);
BENCHMARK(GenerateUuidV7)->RangeMultiplier(2)->Range(1, 1 << 12);

USERVER_NAMESPACE_END
