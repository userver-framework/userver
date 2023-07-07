#include <benchmark/benchmark.h>

#include <userver/utils/encoding/hex.hpp>

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace {

std::string GenerateSource(size_t size) {
  std::string source;
  source.reserve(size);
  for (size_t i = 0; i < size; ++i) {
    source.push_back('a' + i % 26);
  }

  return source;
}

}  // namespace

void to_hex_benchmark(benchmark::State& state) {
  const auto source = GenerateSource(state.range(0));

  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::encoding::ToHex(source));
  }
}
BENCHMARK(to_hex_benchmark)->RangeMultiplier(2)->Range(8, 512);

void to_hex_benchmark_no_alloc(benchmark::State& state) {
  const auto source = GenerateSource(state.range(0));

  std::string out;
  out.reserve(state.range(0) * 2);
  benchmark::DoNotOptimize(out);

  for (auto _ : state) {
    utils::encoding::ToHex(source, out);
  }
}
BENCHMARK(to_hex_benchmark_no_alloc)->RangeMultiplier(2)->Range(8, 512);

USERVER_NAMESPACE_END
