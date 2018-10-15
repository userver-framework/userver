#include <benchmark/benchmark.h>

#include <tracing/jaeger.hpp>
#include <tracing/noop.hpp>
#include <utils/gbench_auxilary.hpp>

namespace {

void tracing_noop_ctr(benchmark::State& state) {
  auto tracer = tracing::MakeNoopTracer();

  for (auto _ : state)
    benchmark::DoNotOptimize(tracer->CreateSpanWithoutParent("name"));
}
BENCHMARK(tracing_noop_ctr);

void tracing_jaeger_ctr(benchmark::State& state) {
  auto tracer = tracing::MakeJaegerLogTracer();

  for (auto _ : state)
    benchmark::DoNotOptimize(tracer->CreateSpanWithoutParent("name"));
}
BENCHMARK(tracing_jaeger_ctr);

}  // namespace
