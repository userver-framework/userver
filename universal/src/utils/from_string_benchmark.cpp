#include <benchmark/benchmark.h>

#include <cstdint>
#include <string>

#include <userver/utils/from_string.hpp>

USERVER_NAMESPACE_BEGIN

template <typename T>
void ConstFromString(benchmark::State& state) {
  std::string str;
  std::size_t len = state.range(0);

  for (std::size_t i = 1; i < len + 1; ++i) {
    str.push_back('0' + i % 10);
  }

  for (auto _ : state) {
    for (std::size_t i = 0; i < 100; ++i) {
      benchmark::DoNotOptimize(utils::FromString<T>(str));
    }
  }
}

BENCHMARK_TEMPLATE(ConstFromString, std::uint64_t)->DenseRange(1, 20, 1);
BENCHMARK_TEMPLATE(ConstFromString, std::uint32_t)->DenseRange(1, 10, 1);
BENCHMARK_TEMPLATE(ConstFromString, std::uint16_t)->DenseRange(1, 5, 1);
BENCHMARK_TEMPLATE(ConstFromString, double)->DenseRange(1, 10, 1);

USERVER_NAMESPACE_END
