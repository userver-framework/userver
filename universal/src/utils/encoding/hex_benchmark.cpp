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

void ToHexBenchmark(benchmark::State& state) {
    const auto source = GenerateSource(state.range(0));

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(utils::encoding::ToHex(source));
    }
}
BENCHMARK(ToHexBenchmark)->RangeMultiplier(2)->Range(8, 512);

void ToHexBenchmarkNoAlloc(benchmark::State& state) {
    const auto source = GenerateSource(state.range(0));

    std::string out;
    out.reserve(state.range(0) * 2);
    benchmark::DoNotOptimize(out);

    for ([[maybe_unused]] auto _ : state) {
        utils::encoding::ToHex(source, out);
    }
}
BENCHMARK(ToHexBenchmarkNoAlloc)->RangeMultiplier(2)->Range(8, 512);

USERVER_NAMESPACE_END
