#include <benchmark/benchmark.h>

#include <userver/tracing/opentelemetry.hpp>

USERVER_NAMESPACE_BEGIN

void ExtractTraceParentDataBench(benchmark::State& state) {
    const std::string input = "00-80e1afed08e019fc1110464cfa66635c-7a085853722dc6d2-01";

    for ([[maybe_unused]] auto _ : state) {
        auto result = tracing::opentelemetry::ExtractTraceParentDataView(input);
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
}

BENCHMARK(ExtractTraceParentDataBench);

USERVER_NAMESPACE_END
