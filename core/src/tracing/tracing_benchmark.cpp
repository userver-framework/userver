#include <benchmark/benchmark.h>

#include <engine/run_in_coro.hpp>
#include <tracing/noop.hpp>
#include <utils/gbench_auxilary.hpp>

namespace {

void tracing_noop_ctr(benchmark::State& state) {
  RunInCoro(
      [&] {
        auto tracer = tracing::MakeNoopTracer();

        for (auto _ : state)
          benchmark::DoNotOptimize(tracer->CreateSpanWithoutParent("name"));
      },
      1);
}
BENCHMARK(tracing_noop_ctr);

}  // namespace
