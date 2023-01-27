#include <deque>
#include <string>

#include <benchmark/benchmark.h>

#include <storages/redis/client_impl.hpp>
#include <storages/redis/util_benchmark.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/utils/rand.hpp>
#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::bench {

using USERVER_NAMESPACE::redis::CommandControl;

namespace {

struct Ping {
  using RequestType = RequestPing;
  ClientPtr client;
  CommandControl cc;

  RequestType operator()(benchmark::State&) { return client->Ping(0, cc); }
};

struct Set {
  using RequestType = RequestSet;
  ClientPtr client;
  CommandControl cc;

  RequestType operator()(benchmark::State& state) {
    auto key = "key" + std::to_string(utils::Rand());
    auto value = std::string(state.range(1), 'x');
    return client->Set(std::move(key), std::move(value), cc);
  }
};

}  // namespace

BENCHMARK_DEFINE_TEMPLATE_F(Redis, PipelineGrind)(benchmark::State& state) {
  RunStandalone(state.range(0), [this, &state] {
    auto request_generator = T{GetClient(), {}};
    std::deque<typename T::RequestType> requests;

    for (auto i = 0; i < state.range(0); ++i) {
      requests.push_back(request_generator(state));
    }

    for (auto _ : state) {
      requests.front().Get();
      requests.pop_front();
      requests.push_back(request_generator(state));
    }

    for (; !requests.empty(); requests.pop_front()) requests.front().Get();
  });
}

BENCHMARK_INSTANTIATE_TEMPLATE_F(Redis, PipelineGrind, Ping)
    ->RangeMultiplier(2)
    ->Range(1, 8);

BENCHMARK_INSTANTIATE_TEMPLATE_F(Redis, PipelineGrind, Set)
    ->RangeMultiplier(2)
    ->Ranges({{1, 4}, {256, 2 << 14}});

}  // namespace storages::redis::bench

USERVER_NAMESPACE_END
