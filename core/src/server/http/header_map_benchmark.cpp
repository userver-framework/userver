#include <benchmark/benchmark.h>

#include <userver/server/http/header_map.hpp>

#include <userver/http/common_headers.hpp>
#include <userver/server/http/http_special_headers.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
using HeaderMap = server::http::HeaderMap;

void PrepareResponse(HeaderMap& headers) {
  headers.Insert(server::http::kTraceIdHeader,
                 "18575ca478104b9fb81460875a2e0a2c");
  headers.Insert(server::http::kSpanIdHeader, "dfea18ffa9c24597");
  headers.Insert(server::http::kYaRequestIdHeader,
                 "767e8d274faf489789cda8a030a98361");
  headers.Insert(server::http::kServerHeader, "userver at some version");
}

}  // namespace

void header_map_for_response(benchmark::State& state) {
  for (auto _ : state) {
    HeaderMap headers{};

    PrepareResponse(headers);

    headers.Erase(server::http::kContentLengthHeader);
    const auto end = headers.end();

    benchmark::DoNotOptimize(headers.find(server::http::kDateHeader) == end);
    benchmark::DoNotOptimize(headers.find(server::http::kContentTypeHeader) ==
                             end);

    std::size_t total_size = 0;
    for (const auto& item : headers) {
      total_size += item.first.size() + item.second.size();
    }
    benchmark::DoNotOptimize(total_size);

    benchmark::DoNotOptimize(headers.find(server::http::kConnectionHeader) ==
                             end);

    benchmark::ClobberMemory();
  }
}
BENCHMARK(header_map_for_response);

USERVER_NAMESPACE_END
