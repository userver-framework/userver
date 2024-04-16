#include <unordered_map>

#include <benchmark/benchmark.h>

#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>

#include <userver/formats/serialize/common_containers.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr char bench_json_data[] = R"({
  "short": "1",
  "long_long_long_long_path": "2",
  "long": {
      "deeply": {
          "deeply": {
              "nested": {
                  "json" : {
                      "value" : {
                          "with" : {
                              "some" : {
                                  "data": "3"
                              }
                          }
                      }
                  }
              }
          }
      }
  },
  "nested_long_long_long_long_path": {
      "deeply": {
          "deeply": {
              "nested": {
                  "json" : {
                      "value" : {
                          "with" : {
                              "some" : {
                                  "data": "4"
                              }
                          }
                      }
                  }
              }
          }
      }
  }
})";

}  // anonymous namespace

void json_path_short(benchmark::State& state) {
  auto json = formats::json::FromString(bench_json_data);

  for ([[maybe_unused]] auto _ : state) {
    const auto res = (json["short"].As<std::string>() == "1");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(json_path_short);

void json_path_long(benchmark::State& state) {
  auto json = formats::json::FromString(bench_json_data);

  for ([[maybe_unused]] auto _ : state) {
    const auto res =
        (json["long_long_long_long_path"].As<std::string>() == "2");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(json_path_long);

void json_path_deeply_nested(benchmark::State& state) {
  auto json = formats::json::FromString(bench_json_data);

  for ([[maybe_unused]] auto _ : state) {
    const auto res = (json["long"]["deeply"]["deeply"]["nested"]["json"]
                          ["value"]["with"]["some"]["data"]
                              .As<std::string>() == "3");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(json_path_deeply_nested);

void json_path_long_and_deeply_nested(benchmark::State& state) {
  auto json = formats::json::FromString(bench_json_data);

  for ([[maybe_unused]] auto _ : state) {
    const auto res =
        (json["nested_long_long_long_long_path"]["deeply"]["deeply"]["nested"]
             ["json"]["value"]["with"]["some"]["data"]
                 .As<std::string>() == "4");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(json_path_long_and_deeply_nested);

formats::json::ValueBuilder Build(size_t count) {
  formats::json::ValueBuilder builder;
  for (size_t i = 0; i < count; i++) builder[std::to_string(i)] = i;
  return builder;
}

void json_object_append(benchmark::State& state) {
  const auto size = state.range(0);
  for ([[maybe_unused]] auto _ : state) {
    benchmark::DoNotOptimize(Build(size));
  }
}
BENCHMARK(json_object_append)->RangeMultiplier(2)->Range(1, 10240);

void json_object_compare(benchmark::State& state) {
  const auto size = state.range(0);
  const auto a = Build(size).ExtractValue();
  const auto b = Build(size).ExtractValue();
  for ([[maybe_unused]] auto _ : state) {
    benchmark::DoNotOptimize(a == b);
  }
}
BENCHMARK(json_object_compare)->RangeMultiplier(2)->Range(1, 1024);

formats::json::ValueBuilder BuildNocheck(size_t count) {
  formats::json::ValueBuilder builder;
  for (size_t i = 0; i < count; i++) {
    builder.EmplaceNocheck(std::to_string(i), i);
  }
  return builder;
}

void json_object_append_nocheck(benchmark::State& state) {
  const auto size = state.range(0);
  for ([[maybe_unused]] auto _ : state) {
    benchmark::DoNotOptimize(BuildNocheck(size));
  }
}
BENCHMARK(json_object_append_nocheck)->RangeMultiplier(2)->Range(1, 128);

void json_object_from_unordered(benchmark::State& state) {
  const auto size = state.range(0);

  std::unordered_map<std::string, int> map;
  for (int i = 0; i < size; i++) map[std::to_string(i)] = i;

  for ([[maybe_unused]] auto _ : state) {
    benchmark::DoNotOptimize(formats::json::ValueBuilder(map));
  }
}
BENCHMARK(json_object_from_unordered)->RangeMultiplier(2)->Range(1, 1024);

void json_object_from_unordered_strong_typedef(benchmark::State& state) {
  const auto size = state.range(0);

  using MyString = utils::StrongTypedef<class Tag, std::string>;
  std::unordered_map<MyString, int> map;
  for (int i = 0; i < size; i++) map[MyString{std::to_string(i)}] = i;

  for ([[maybe_unused]] auto _ : state) {
    benchmark::DoNotOptimize(formats::json::ValueBuilder(map));
  }
}
BENCHMARK(json_object_from_unordered_strong_typedef)
    ->RangeMultiplier(2)
    ->Range(1, 1024);

void json_object_wide_object_operator_equals(benchmark::State& state) {
  const std::size_t size = state.range(0);

  formats::json::ValueBuilder builder{formats::json::Type::kObject};
  for (std::size_t i = 0; i < size; ++i) {
    std::string some_common_prefix(5 + i % 8, 'a');
    some_common_prefix.append(std::to_string(i));
    builder[std::move(some_common_prefix)] = i;
  }
  const auto first_value = builder.ExtractValue();
  const auto second_value = first_value.Clone();

  for ([[maybe_unused]] auto _ : state) {
    benchmark::DoNotOptimize(first_value == second_value);
  }
}
BENCHMARK(json_object_wide_object_operator_equals)
    ->DenseRange(4, 16, 4)
    ->Range(32, 8192)
    ->RangeMultiplier(2);

USERVER_NAMESPACE_END
