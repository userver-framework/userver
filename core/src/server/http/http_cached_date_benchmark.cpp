#include <benchmark/benchmark.h>

#include <server/http/http_cached_date.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

void HttpGetCachedDateBenchmark(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(server::http::impl::GetCachedDate());
    }
}
BENCHMARK(HttpGetCachedDateBenchmark);

void HttpMakeDateBenchmark(benchmark::State& state) {
    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(server::http::impl::MakeHttpDate(utils::datetime::WallCoarseClock::now()));
    }
}
BENCHMARK(HttpMakeDateBenchmark);

USERVER_NAMESPACE_END
