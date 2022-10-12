#include <benchmark/benchmark.h>

#include <server/http/http_cached_date.hpp>

USERVER_NAMESPACE_BEGIN

void http_date_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(server::http::GetHttpDate());
  }
}
BENCHMARK(http_date_benchmark);

void http_cached_date_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(server::http::GetCachedHttpDate());
  }
}
BENCHMARK(http_cached_date_benchmark);

USERVER_NAMESPACE_END
