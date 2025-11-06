#include <benchmark/benchmark.h>

#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

void MakeUrl(benchmark::State& state, std::size_t size) {
    const std::string path = "http://example.com/v1/something";
    std::string spaces(size, ' ');
    std::string latins(size, 'a');
    std::string latins_with_spaces = spaces + latins;
    for ([[maybe_unused]] auto _ : state) {
        const auto result = http::MakeUrl(path, {{"a", latins}, {"b", spaces}, {"c", latins_with_spaces}, {"d", ""}});
        benchmark::DoNotOptimize(result);
    }
}

void MakeUrlTiny(benchmark::State& state) { MakeUrl(state, 5); }
BENCHMARK(MakeUrlTiny);

void MakeUrlSmall(benchmark::State& state) { MakeUrl(state, 50); }
BENCHMARK(MakeUrlSmall);

void MakeUrlBig(benchmark::State& state) { MakeUrl(state, 5000); }
BENCHMARK(MakeUrlBig);

void MakeQuery(benchmark::State& state) {
    http::Args query_args;
    const auto agrs_count = state.range(0);
    for (int i = 0; i < agrs_count; i++) {
        const std::string str = std::to_string(i);
        query_args[str] = str;
    }
    for ([[maybe_unused]] auto _ : state) {
        const auto result = http::MakeQuery(query_args);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(MakeQuery)->RangeMultiplier(2)->Range(1, 256);

void MakeUrlWithPathArgsValueSizes(benchmark::State& state, std::size_t size) {
    const std::string path_template = "http://example.com/v1/{resource}/{id}";
    std::string large_value(size, 'x');

    http::PathArgs path_args = {{"resource", "users"}, {"id", large_value}};
    http::Args query_args = {{"param", large_value}};

    for ([[maybe_unused]] auto _ : state) {
        const auto result = http::MakeUrlWithPathArgs(path_template, path_args, query_args);
        benchmark::DoNotOptimize(result);
    }
}

void MakeUrlWithPathArgsSmallValues(benchmark::State& state) { MakeUrlWithPathArgsValueSizes(state, 50); }
BENCHMARK(MakeUrlWithPathArgsSmallValues);

void MakeUrlWithPathArgsMediumValues(benchmark::State& state) { MakeUrlWithPathArgsValueSizes(state, 500); }
BENCHMARK(MakeUrlWithPathArgsMediumValues);

void MakeUrlWithPathArgsLargeValues(benchmark::State& state) { MakeUrlWithPathArgsValueSizes(state, 5000); }
BENCHMARK(MakeUrlWithPathArgsLargeValues);

void MakeUrlWithPathArgsTemplateComplexity(benchmark::State& state) {
    std::string path_template;
    http::PathArgs path_args;

    const auto template_vars_count = state.range(0);
    path_template = "http://example.com";

    for (int i = 0; i < template_vars_count; i++) {
        const std::string key = "p" + std::to_string(i);
        path_template += "/segment_{" + key + "}";
        path_args[key] = "value" + std::to_string(i);
    }

    for ([[maybe_unused]] auto _ : state) {
        const auto result = http::MakeUrlWithPathArgs(path_template, path_args);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(MakeUrlWithPathArgsTemplateComplexity)->Range(1, 256);

USERVER_NAMESPACE_END
