#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/tracing/noop.hpp>
#include <userver/tracing/opentracing.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void tracing_noop_ctr(benchmark::State& state) {
  engine::RunStandalone([&] {
    auto tracer = tracing::MakeNoopTracer("test_service");

    for (auto _ : state)
      benchmark::DoNotOptimize(tracer->CreateSpanWithoutParent("name"));
  });
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
  engine::RunStandalone([&] {
    auto tracer = tracing::MakeNoopTracer("test_service");
    tracing::SetOpentracingLogger(logger);
    for (auto _ : state) {
      benchmark::DoNotOptimize(GetSpanWithOpentracingHttpTags(tracer));
    }
  });
}
BENCHMARK(tracing_opentracing_ctr);

}  // namespace

USERVER_NAMESPACE_END
