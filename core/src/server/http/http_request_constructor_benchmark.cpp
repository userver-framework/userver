#include <benchmark/benchmark.h>

#include <server/http/http_request_constructor.hpp>
#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void http_request_constructor_url_decode(benchmark::State& state) {
  std::string tmp = "1";
  std::string input;

  for (int64_t i = 0; i < state.range(0); i++) input += tmp;

  for (auto _ : state)
    benchmark::DoNotOptimize(USERVER_NAMESPACE::http::parser::UrlDecode(input));
}
}  // namespace
BENCHMARK(http_request_constructor_url_decode)
    ->RangeMultiplier(2)
    ->Range(1, 1024);

USERVER_NAMESPACE_END
