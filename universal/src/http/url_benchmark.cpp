#include <benchmark/benchmark.h>

#include <userver/http/url.hpp>

USERVER_NAMESPACE_BEGIN

void make_url(benchmark::State& state, std::size_t size) {
    const std::string path = "http://example.com/v1/something";
    std::string spaces(size, ' ');
    std::string latins(size, 'a');
    std::string latins_with_spaces = spaces + latins;
    for ([[maybe_unused]] auto _ : state) {
        const auto result = http::MakeUrl(path, {{"a", latins}, {"b", spaces}, {"c", latins_with_spaces}, {"d", ""}});
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
        const std::string str = std::to_string(i);
        query_args[str] = str;
    }
    for ([[maybe_unused]] auto _ : state) {
        const auto result = http::MakeQuery(query_args);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(make_query)->RangeMultiplier(2)->Range(1, 256);

void make_url_with_path_args_value_sizes(benchmark::State& state, std::size_t size) {
    const std::string path_template = "http://example.com/v1/{resource}/{id}";
    std::string large_value(size, 'x');

    http::PathArgs path_args = {{"resource", "users"}, {"id", large_value}};
    http::Args query_args = {{"param", large_value}};

    for ([[maybe_unused]] auto _ : state) {
        const auto result = http::MakeUrlWithPathArgs(path_template, path_args, query_args);
        benchmark::DoNotOptimize(result);
    }
}

void make_url_with_path_args_small_values(benchmark::State& state) { make_url_with_path_args_value_sizes(state, 50); }
BENCHMARK(make_url_with_path_args_small_values);

void make_url_with_path_args_medium_values(benchmark::State& state) { make_url_with_path_args_value_sizes(state, 500); }
BENCHMARK(make_url_with_path_args_medium_values);

void make_url_with_path_args_large_values(benchmark::State& state) { make_url_with_path_args_value_sizes(state, 5000); }
BENCHMARK(make_url_with_path_args_large_values);

void make_url_with_path_args_template_complexity(benchmark::State& state) {
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
BENCHMARK(make_url_with_path_args_template_complexity)->Range(1, 256);

USERVER_NAMESPACE_END
