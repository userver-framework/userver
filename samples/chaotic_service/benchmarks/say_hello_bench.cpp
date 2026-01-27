#include <userver/utest/using_namespace_userver.hpp>

#include "say_hello.hpp"

#include <string_view>

#include <benchmark/benchmark.h>
#include <userver/engine/run_standalone.hpp>

void HelloBenchmark(benchmark::State& state) {
    engine::RunStandalone([&] {
        const ::samples::hello::HelloResponseBody response{"userver"};
        const ::samples::hello::HelloRequestBody request{"userver"};

        for (auto _ : state) {
            auto result = samples::hello::SayHelloTo(request);
            benchmark::DoNotOptimize(result);
        }
    });
}

BENCHMARK(HelloBenchmark);
