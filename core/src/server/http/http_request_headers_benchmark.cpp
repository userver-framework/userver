#include <benchmark/benchmark.h>

#include <server/http/http_request_constructor.hpp>
#include <userver/http/predefined_header.hpp>

#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using PredefinedHeader = http::headers::PredefinedHeader;

constexpr std::size_t kHeadersCount = 32;
constexpr PredefinedHeader kHeadersArray[kHeadersCount] = {
    PredefinedHeader{"TestHeader0"},  PredefinedHeader{"TestHeader1"},
    PredefinedHeader{"TestHeader2"},  PredefinedHeader{"TestHeader3"},
    PredefinedHeader{"TestHeader4"},  PredefinedHeader{"TestHeader5"},
    PredefinedHeader{"TestHeader6"},  PredefinedHeader{"TestHeader7"},
    PredefinedHeader{"TestHeader8"},  PredefinedHeader{"TestHeader9"},
    PredefinedHeader{"TestHeader10"}, PredefinedHeader{"TestHeader11"},
    PredefinedHeader{"TestHeader12"}, PredefinedHeader{"TestHeader13"},
    PredefinedHeader{"TestHeader14"}, PredefinedHeader{"TestHeader15"},
    PredefinedHeader{"TestHeader16"}, PredefinedHeader{"TestHeader17"},
    PredefinedHeader{"TestHeader18"}, PredefinedHeader{"TestHeader19"},
    PredefinedHeader{"TestHeader20"}, PredefinedHeader{"TestHeader21"},
    PredefinedHeader{"TestHeader22"}, PredefinedHeader{"TestHeader23"},
    PredefinedHeader{"TestHeader24"}, PredefinedHeader{"TestHeader25"},
    PredefinedHeader{"TestHeader26"}, PredefinedHeader{"TestHeader27"},
    PredefinedHeader{"TestHeader28"}, PredefinedHeader{"TestHeader29"},
    PredefinedHeader{"TestHeader30"}, PredefinedHeader{"TestHeader31"},
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
  for (const auto& header : kHeadersArray) map[header] = "1";

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
