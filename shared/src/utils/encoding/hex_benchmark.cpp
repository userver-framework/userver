#include <benchmark/benchmark.h>

#include <userver/utils/encoding/hex.hpp>

#include <string_view>

USERVER_NAMESPACE_BEGIN

void to_hex_benchmark(benchmark::State& state) {
  constexpr std::string_view kUuid = "21e30c92afe54396";

  for (auto _ : state) {
    benchmark::DoNotOptimize(utils::encoding::ToHex(kUuid));
  }
}
BENCHMARK(to_hex_benchmark);

USERVER_NAMESPACE_END
