#include <userver/utils/consteval_map.hpp>

#include <unordered_map>

#include <benchmark/benchmark.h>

USERVER_NAMESPACE_BEGIN

constexpr auto kSmallConstinitMapping =
    utils::MakeConsinitMap<std::string_view, int>({
        {"hello", 1},
        {"world", 2},
        {"z", 42},
    });

const auto kSmallUnorderedMapping = std::unordered_map<std::string_view, int>{
    {"hello", 1},
    {"world", 2},
    {"z", 42},
};

constexpr auto kMediumConstinitMapping =
    utils::MakeConsinitMap<std::string_view, int>({
        {"hello", 1},
        {"world", 2},
        {"a", 3},
        {"b", 4},
        {"c", 5},
        {"d", 6},
        {"e", 7},
        {"f", 8},
        {"z", 42},
    });

const auto kMediumUnorderedMapping = std::unordered_map<std::string_view, int>{
    {"hello", 1}, {"world", 2}, {"a", 3}, {"b", 4},  {"c", 5},
    {"d", 6},     {"e", 7},     {"f", 8}, {"z", 42},
};

constexpr auto kHugeConstinitMapping =
    utils::MakeConsinitMap<std::string_view, int>({
        {"aaaaaaaaaaaaaaaa_hello", 1}, {"aaaaaaaaaaaaaaaa_world", 2},
        {"aaaaaaaaaaaaaaaa_a", 3},     {"aaaaaaaaaaaaaaaa_b", 4},
        {"aaaaaaaaaaaaaaaa_c", 5},     {"aaaaaaaaaaaaaaaa_d", 6},
        {"aaaaaaaaaaaaaaaa_e", 7},     {"aaaaaaaaaaaaaaaa_f", 8},
        {"aaaaaaaaaaaaaaaa_f1", 8},    {"aaaaaaaaaaaaaaaa_f2", 8},
        {"aaaaaaaaaaaaaaaa_f3", 8},    {"aaaaaaaaaaaaaaaa_f4", 8},
        {"aaaaaaaaaaaaaaaa_f5", 8},    {"aaaaaaaaaaaaaaaa_f6", 8},
        {"aaaaaaaaaaaaaaaa_f7", 8},    {"aaaaaaaaaaaaaaaa_f8", 8},
        {"aaaaaaaaaaaaaaaa_f9", 8},    {"aaaaaaaaaaaaaaaa_z", 42},
        {"aaaaaaaaaaaaaaaa_z1", 42},   {"aaaaaaaaaaaaaaaa_z2", 42},
        {"aaaaaaaaaaaaaaaa_z3", 42},   {"aaaaaaaaaaaaaaaa_z4", 42},
        {"aaaaaaaaaaaaaaaa_z5", 42},   {"aaaaaaaaaaaaaaaa_z6", 42},
        {"aaaaaaaaaaaaaaaa_z7", 42},   {"aaaaaaaaaaaaaaaa_z8", 42},
        {"aaaaaaaaaaaaaaaa_z9", 42},   {"aaaaaaaaaaaaaaaa_x", 42},
        {"aaaaaaaaaaaaaaaa_x1", 42},   {"aaaaaaaaaaaaaaaa_x2", 42},
        {"aaaaaaaaaaaaaaaa_x3", 42},   {"aaaaaaaaaaaaaaaa_x4", 42},
        {"aaaaaaaaaaaaaaaa_x5", 42},   {"aaaaaaaaaaaaaaaa_x6", 42},
        {"aaaaaaaaaaaaaaaa_x7", 42},   {"aaaaaaaaaaaaaaaa_x8", 42},
        {"aaaaaaaaaaaaaaaa_x9", 42},
    });

const auto kHugeUnorderedMapping = std::unordered_map<std::string_view, int>{
    {"aaaaaaaaaaaaaaaa_hello", 1}, {"aaaaaaaaaaaaaaaa_world", 2},
    {"aaaaaaaaaaaaaaaa_a", 3},     {"aaaaaaaaaaaaaaaa_b", 4},
    {"aaaaaaaaaaaaaaaa_c", 5},     {"aaaaaaaaaaaaaaaa_d", 6},
    {"aaaaaaaaaaaaaaaa_e", 7},     {"aaaaaaaaaaaaaaaa_f", 8},
    {"aaaaaaaaaaaaaaaa_f1", 8},    {"aaaaaaaaaaaaaaaa_f2", 8},
    {"aaaaaaaaaaaaaaaa_f3", 8},    {"aaaaaaaaaaaaaaaa_f4", 8},
    {"aaaaaaaaaaaaaaaa_f5", 8},    {"aaaaaaaaaaaaaaaa_f6", 8},
    {"aaaaaaaaaaaaaaaa_f7", 8},    {"aaaaaaaaaaaaaaaa_f8", 8},
    {"aaaaaaaaaaaaaaaa_f9", 8},    {"aaaaaaaaaaaaaaaa_z", 42},
    {"aaaaaaaaaaaaaaaa_z1", 42},   {"aaaaaaaaaaaaaaaa_z2", 42},
    {"aaaaaaaaaaaaaaaa_z3", 42},   {"aaaaaaaaaaaaaaaa_z4", 42},
    {"aaaaaaaaaaaaaaaa_z5", 42},   {"aaaaaaaaaaaaaaaa_z6", 42},
    {"aaaaaaaaaaaaaaaa_z7", 42},   {"aaaaaaaaaaaaaaaa_z8", 42},
    {"aaaaaaaaaaaaaaaa_z9", 42},   {"aaaaaaaaaaaaaaaa_x", 42},
    {"aaaaaaaaaaaaaaaa_x1", 42},   {"aaaaaaaaaaaaaaaa_x2", 42},
    {"aaaaaaaaaaaaaaaa_x3", 42},   {"aaaaaaaaaaaaaaaa_x4", 42},
    {"aaaaaaaaaaaaaaaa_x5", 42},   {"aaaaaaaaaaaaaaaa_x6", 42},
    {"aaaaaaaaaaaaaaaa_x7", 42},   {"aaaaaaaaaaaaaaaa_x8", 42},
    {"aaaaaaaaaaaaaaaa_x9", 42},
};

void MappingSmallConstinit(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("hello"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("world"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("a"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("b"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("c"));

    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("d"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("e"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("f"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("z"));
    benchmark::DoNotOptimize(kSmallConstinitMapping.FindOrNullptr("z"));
  }
}
BENCHMARK(MappingSmallConstinit);

void MappingSmallUnordered(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("hello"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("world"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("a"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("b"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("c"));

    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("d"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("e"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("f"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("z"));
    benchmark::DoNotOptimize(kSmallUnorderedMapping.find("z"));
  }
}
BENCHMARK(MappingSmallUnordered);

void MappingMediumConstinit(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("hello"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("world"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("a"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("b"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("c"));

    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("d"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("e"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("f"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("z"));
    benchmark::DoNotOptimize(kMediumConstinitMapping.FindOrNullptr("z"));
  }
}
BENCHMARK(MappingMediumConstinit);

void MappingMediumUnordered(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("hello"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("world"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("a"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("b"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("c"));

    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("d"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("e"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("f"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("z"));
    benchmark::DoNotOptimize(kMediumUnorderedMapping.find("z"));
  }
}
BENCHMARK(MappingMediumUnordered);

void MappingHugeConstinit(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_hello"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_world"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_a"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_b"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_c"));

    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_d"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_e"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_f9"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_z"));
    benchmark::DoNotOptimize(
        kHugeConstinitMapping.FindOrNullptr("aaaaaaaaaaaaaaaa_z9"));
  }
}
BENCHMARK(MappingHugeConstinit);

void MappingHugeUnordered(benchmark::State& state) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(
        kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_hello"));
    benchmark::DoNotOptimize(
        kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_world"));
    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_a"));
    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_b"));
    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_c"));

    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_d"));
    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_e"));
    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_f9"));
    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_z"));
    benchmark::DoNotOptimize(kHugeUnorderedMapping.find("aaaaaaaaaaaaaaaa_z9"));
  }
}
BENCHMARK(MappingHugeUnordered);

USERVER_NAMESPACE_END
