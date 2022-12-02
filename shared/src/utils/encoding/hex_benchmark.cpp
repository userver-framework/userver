#include <benchmark/benchmark.h>

#include <userver/utils/encoding/hex.hpp>

USERVER_NAMESPACE_BEGIN

void to_hex_benchmark(benchmark::State& state) {
  std::string source;
  source.reserve(state.range(0));
  for (ssize_t i = 0; i < state.range(0); ++i) {
    source.push_back('a' + i % 26);
  }

  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::encoding::ToHex(source));
  }
}
BENCHMARK(to_hex_benchmark)->RangeMultiplier(2)->Range(8, 512);

USERVER_NAMESPACE_END
