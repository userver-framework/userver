#include <benchmark/benchmark.h>

#include <logging/impl/formatters/tskv.hpp>

USERVER_NAMESPACE_BEGIN

void TskvFormatterConstructor(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        logging::impl::formatters::Tskv
            formatter(logging::Level::kDebug, logging::Format::kTskv, utils::impl::SourceLocation::Current());
        benchmark::DoNotOptimize(&formatter);
    }
}
BENCHMARK(TskvFormatterConstructor);

void TskvFormatterAndTags(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        logging::impl::formatters::Tskv
            formatter(logging::Level::kDebug, logging::Format::kTskv, utils::impl::SourceLocation::Current());
        formatter.AddTag("key1", std::string_view{"some_value"});
        formatter.AddTag("key2", std::string_view{"some value2"});
        formatter.AddTag("key 3", std::string_view{"some value2"});
        benchmark::DoNotOptimize(&formatter);
    }
}
BENCHMARK(TskvFormatterAndTags);

USERVER_NAMESPACE_END
