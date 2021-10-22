#include <benchmark/benchmark.h>

#include <userver/formats/bson.hpp>
#include <userver/formats/bson/serialize.hpp>
#include <userver/formats/json.hpp>

USERVER_NAMESPACE_BEGIN

namespace {
constexpr char bench_bson_data[] = R"({
  "short": "1",
  "very_long_long_long_long_path": "2",
  "long": {
      "deeply": {
          "deeply": {
              "nested": {
                  "bson" : {
                      "value" : {
                          "with" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with1" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with2" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with3" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with4" : {
                              "some" : {
                                  "data": "3"
                              }
                          }
                      }
                  }
              },
              "nested2": {
                  "bson" : {
                      "value" : {
                          "with" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with1" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with2" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with3" : {
                              "some" : {
                                  "data": "3"
                              }
                          },
                          "with4" : {
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
  "nested_very_long_long_long_long_path": {
      "deeply": {
          "deeply": {
              "nested": {
                  "bson" : {
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

void bson_path_first_access(benchmark::State& state) {
  for (auto _ : state) {
    state.PauseTiming();
    auto bson = formats::bson::FromJsonString(bench_bson_data);
    state.ResumeTiming();

    const auto res =
        (bson["nested_very_long_long_long_long_path"]["deeply"]["deeply"]
             ["nested"]["bson"]["value"]["with"]["some"]["data"]
                 .As<std::string>() == "4");
    benchmark::DoNotOptimize(res);
    if (!res) throw std::runtime_error("unexpected");
  }
}
BENCHMARK(bson_path_first_access);

USERVER_NAMESPACE_END
