#include <benchmark/benchmark.h>

#include <server/http/http_cached_date.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>

USERVER_NAMESPACE_BEGIN

void http_get_cached_date_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(server::http::impl::GetCachedDate());
  }
}
BENCHMARK(http_get_cached_date_benchmark);

void http_make_date_benchmark(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(server::http::impl::MakeHttpDate(
        utils::datetime::WallCoarseClock::now()));
  }
}
BENCHMARK(http_make_date_benchmark);

USERVER_NAMESPACE_END
