#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/logging/impl/logger_base.hpp>
#include <userver/logging/null_logger.hpp>
#include <userver/tracing/tracer.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class NoopLogger : public logging::impl::TextLogger {
public:
    NoopLogger() noexcept : TextLogger(logging::Format::kRaw) { SetLevel(logging::Level::kInfo); }
    void Log(logging::Level, logging::impl::formatters::LoggerItemRef) override {}
    void Flush() override {}
};

void tracing_noop_ctr(benchmark::State& state) {
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            tracing::Span tmp = tracing::Span::MakeRootSpan("name");
            tmp.SetLogLevel(logging::Level::kNone);
            benchmark::DoNotOptimize(tmp.GetSpanId());
        }
    });
}
BENCHMARK(tracing_noop_ctr);

void tracing_happy_log(benchmark::State& state) {
    const logging::DefaultLoggerGuard guard{std::make_shared<NoopLogger>()};

    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            const auto tmp = tracing::Span::MakeRootSpan("name");
            benchmark::DoNotOptimize(tmp.GetSpanId());
        }
    });
}
BENCHMARK(tracing_happy_log);

tracing::Span GetSpanWithOpentracingHttpTags() {
    auto span = tracing::Span::MakeRootSpan("name");
    span.AddTag("meta_code", 200);
    span.AddTag("error", false);
    span.AddTag("http.url", "http://example.com/example");
    return span;
}

void tracing_opentracing_ctr(benchmark::State& state) {
    auto logger = logging::MakeNullLogger();
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            const tracing::Span tmp = GetSpanWithOpentracingHttpTags();
            benchmark::DoNotOptimize(tmp.GetSpanId());
        }
    });
}
BENCHMARK(tracing_opentracing_ctr);

}  // namespace

USERVER_NAMESPACE_END
