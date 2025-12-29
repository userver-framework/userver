#include <benchmark/benchmark.h>

#include <server/http/http_request_constructor.hpp>
#include <utils/gbench_auxiliary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void HttpRequestConstructorUrlDecode(benchmark::State& state) {
    const std::string tmp = "1";
    std::string input;

    for (int64_t i = 0; i < state.range(0); i++) {
        input += tmp;
    }

    for ([[maybe_unused]] auto _ : state) {
        benchmark::DoNotOptimize(USERVER_NAMESPACE::http::parser::UrlDecode(input));
    }
}
}  // namespace
BENCHMARK(HttpRequestConstructorUrlDecode)->RangeMultiplier(2)->Range(1, 1024);

USERVER_NAMESPACE_END
