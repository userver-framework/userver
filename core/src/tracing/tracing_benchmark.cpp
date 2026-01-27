#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/tracing/tracer.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void TracingNoopCtr(benchmark::State& state) {
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            tracing::Span tmp = tracing::Span::MakeRootSpan("name");
            tmp.SetLogLevel(logging::Level::kNone);
            benchmark::DoNotOptimize(tmp.GetSpanId());
        }
    });
}
BENCHMARK(TracingNoopCtr);

void TracingHappyLog(benchmark::State& state) {
    const logging::DefaultLoggerGuard guard{logging::impl::MakeNoopLoggerForTests()};

    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            const auto tmp = tracing::Span::MakeRootSpan("name");
            benchmark::DoNotOptimize(tmp.GetSpanId());
        }
    });
}
BENCHMARK(TracingHappyLog);

tracing::Span GetSpanWithOpentracingHttpTags() {
    auto span = tracing::Span::MakeRootSpan("name");
    span.AddTag("meta_code", 200);
    span.AddTag("error", false);
    span.AddTag("http.url", "http://example.com/example");
    return span;
}

void TracingOpentracingCtr(benchmark::State& state) {
    auto logger = logging::MakeNullLogger();
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            const tracing::Span tmp = GetSpanWithOpentracingHttpTags();
            benchmark::DoNotOptimize(tmp.GetSpanId());
        }
    });
}
BENCHMARK(TracingOpentracingCtr);

}  // namespace

USERVER_NAMESPACE_END
