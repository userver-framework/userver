#include <benchmark/benchmark.h>

#include <userver/engine/run_in_coro.hpp>
#include <userver/tracing/noop.hpp>
#include <userver/tracing/opentracing.hpp>
#include <utils/gbench_auxilary.hpp>

namespace {

void tracing_noop_ctr(benchmark::State& state) {
  RunInCoro(
      [&] {
        auto tracer = tracing::MakeNoopTracer("test_service");

        for (auto _ : state)
          benchmark::DoNotOptimize(tracer->CreateSpanWithoutParent("name"));
      },
      1);
}
BENCHMARK(tracing_noop_ctr);

tracing::Span GetSpanWithOpentracingHttpTags(tracing::TracerPtr tracer) {
  auto span = tracer->CreateSpanWithoutParent("name");
  span.AddTag("meta_code", 200);
  span.AddTag("error", false);
  span.AddTag("http.url", "http://example.com/example");
  return span;
}

void tracing_opentracing_ctr(benchmark::State& state) {
  logging::LoggerPtr logger = logging::MakeNullLogger("opentracing");
  RunInCoro(
      [&] {
        auto tracer = tracing::MakeNoopTracer("test_service");
        tracing::SetOpentracingLogger(logger);
        for (auto _ : state) {
          benchmark::DoNotOptimize(GetSpanWithOpentracingHttpTags(tracer));
        }
      },
      1);
}
BENCHMARK(tracing_opentracing_ctr);

}  // namespace
