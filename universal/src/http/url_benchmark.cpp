#include <benchmark/benchmark.h>

#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

void make_url(benchmark::State& state, std::size_t size) {
  std::string path = "http://example.com/v1/something";
  std::string spaces(size, ' ');
  std::string latins(size, 'a');
  std::string latins_with_spaces = spaces + latins;
  for (auto _ : state) {
    const auto result = http::MakeUrl(
        path,
        {{"a", latins}, {"b", spaces}, {"c", latins_with_spaces}, {"d", ""}});
    benchmark::DoNotOptimize(result);
  }
}

void make_url_tiny(benchmark::State& state) { make_url(state, 5); }
BENCHMARK(make_url_tiny);

void make_url_small(benchmark::State& state) { make_url(state, 50); }
BENCHMARK(make_url_small);

void make_url_big(benchmark::State& state) { make_url(state, 5000); }
BENCHMARK(make_url_big);

void make_query(benchmark::State& state) {
  http::Args query_args;
  const auto agrs_count = state.range(0);
  for (int i = 0; i < agrs_count; i++) {
    std::string str = std::to_string(i);
    query_args[str] = str;
  }
  for (auto _ : state) {
    const auto result = http::MakeQuery(query_args);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(make_query)->RangeMultiplier(2)->Range(1, 256);

USERVER_NAMESPACE_END
