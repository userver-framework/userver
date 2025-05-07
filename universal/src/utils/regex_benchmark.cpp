#include <userver/utils/regex.hpp>

#include <benchmark/benchmark.h>

#include <utils/gbench_auxiliary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// In results of a multithreaded benchmark,
// "Time" is how much time the whole system spends to make an iteration, regardless of the thread it's run on.
// "CPU" is how much CPU time is spent per iteration, regardless of the thread it's run on.
// In a perfect world, "Time" ~ 1/min(threads,N_CORES), and "CPU" is constant.
void SetupRegexBenchmark(benchmark::internal::Benchmark* benchmark) {
    benchmark->UseRealTime()
        ->DenseThreadRange(1, 4)
        ->Threads(6)
        ->Threads(8)
        ->Threads(12)
        ->RangeMultiplier(2)
        ->ThreadRange(16, 128);
}

void UtilsRegexCreateAndUse(benchmark::State& state) {
    constexpr std::string_view string{"[2000-01-01 00:00:00.000]"};

    for ([[maybe_unused]] auto _ : state) {
        const utils::regex regex{R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3}\])"};
        const bool matches = utils::regex_match(string, regex);
        benchmark::DoNotOptimize(matches);
    }
}
BENCHMARK(UtilsRegexCreateAndUse)->Apply(SetupRegexBenchmark);

void UtilsRegexUse(benchmark::State& state) {
    const utils::regex regex{R"(\[\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}.\d{3}\])"};

    for ([[maybe_unused]] auto _ : state) {
        constexpr std::string_view string{"[2000-01-01 00:00:00.000]"};
        const bool matches = utils::regex_match(string, regex);
        benchmark::DoNotOptimize(matches);
    }
}
BENCHMARK(UtilsRegexUse)->Apply(SetupRegexBenchmark);

}  // namespace

USERVER_NAMESPACE_END
