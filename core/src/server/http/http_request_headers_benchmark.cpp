#include <benchmark/benchmark.h>

#include <server/http/http_request_constructor.hpp>

#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr std::size_t kHeadersCount = 32;
const char* kHeadersArray[kHeadersCount] = {
    "TestHeader0",  "TestHeader1",  "TestHeader2",  "TestHeader3",
    "TestHeader4",  "TestHeader5",  "TestHeader6",  "TestHeader7",
    "TestHeader8",  "TestHeader9",  "TestHeader10", "TestHeader11",
    "TestHeader12", "TestHeader13", "TestHeader14", "TestHeader15",
    "TestHeader16", "TestHeader17", "TestHeader18", "TestHeader19",
    "TestHeader20", "TestHeader21", "TestHeader22", "TestHeader23",
    "TestHeader24", "TestHeader25", "TestHeader26", "TestHeader27",
    "TestHeader28", "TestHeader29", "TestHeader30", "TestHeader31",
};

void http_request_headers_insert(benchmark::State& state) {
  for (auto _ : state) {
    server::http::HttpRequest::HeadersMap map;

    for (int i = 0; i < state.range(0); i++) map[kHeadersArray[i]] = "1";

    benchmark::DoNotOptimize(map);
  }
}

void http_request_headers_get(benchmark::State& state) {
  server::http::HttpRequest::HeadersMap map;
  for (std::size_t i = 0; i < kHeadersCount; i++) map[kHeadersArray[i]] = "1";

  std::size_t i = 0;
  for (auto _ : state) {
    if (++i == kHeadersCount) i = 0;
    benchmark::DoNotOptimize(map.find(kHeadersArray[i]));
  }
}

}  // namespace
BENCHMARK(http_request_headers_insert)
    ->RangeMultiplier(2)
    ->Range(1, kHeadersCount);

BENCHMARK(http_request_headers_get);

USERVER_NAMESPACE_END
