#include <benchmark/benchmark.h>

#include <server/http/http_request_constructor.hpp>
#include <utils/gbench_auxilary.hpp>

namespace {

void http_request_constructor_url_decode(benchmark::State& state) {
  std::string tmp = "1";
  std::string input;

  for (int64_t i = 0; i < state.range(0); i++) input += tmp;

  const char* begin = input.c_str();
  const char* end = begin + strlen(begin);

  using server::http::HttpRequestConstructor;

  for (auto _ : state)
    benchmark::DoNotOptimize(HttpRequestConstructor::UrlDecode(
        begin, end, HttpRequestConstructor::DecodeMode::Query));
}
}  // namespace
BENCHMARK(http_request_constructor_url_decode)
    ->RangeMultiplier(2)
    ->Range(1, 1024);
