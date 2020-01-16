#include <benchmark/benchmark.h>

#include <formats/json/exception.hpp>
#include <formats/json/serialize.hpp>
#include <formats/json/value_builder.hpp>

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

  for (auto _ : state) {
    const auto res = (json["short"].As<std::string>() == "1");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(json_path_short);

void json_path_long(benchmark::State& state) {
  auto json = formats::json::FromString(bench_json_data);

  for (auto _ : state) {
    const auto res =
        (json["long_long_long_long_path"].As<std::string>() == "2");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(json_path_long);

void json_path_deeply_nested(benchmark::State& state) {
  auto json = formats::json::FromString(bench_json_data);

  for (auto _ : state) {
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

  for (auto _ : state) {
    const auto res =
        (json["nested_long_long_long_long_path"]["deeply"]["deeply"]["nested"]
             ["json"]["value"]["with"]["some"]["data"]
                 .As<std::string>() == "4");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(json_path_long_and_deeply_nested);
