#include <deque>
#include <string>

#include <benchmark/benchmark.h>

#include <storages/redis/client_impl.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/utils/rand.hpp>
#include <utils/gbench_auxilary.hpp>

#include "redis_fixture.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages::redis::bench {

using USERVER_NAMESPACE::redis::CommandControl;

namespace {

struct Ping {
  using RequestType = RequestPing;
  ClientPtr client;
  CommandControl cc;

  RequestType operator()(benchmark::State&) const {
    return client->Ping(0, cc);
  }
};

struct Set {
  using RequestType = RequestSet;
  ClientPtr client;
  CommandControl cc;

  RequestType operator()(benchmark::State& state) const {
    auto key = "key" + std::to_string(utils::Rand() % state.range(0));
    auto value = std::string(state.range(1), 'x');
    return client->Set(std::move(key), std::move(value), cc);
  }
};

}  // namespace

BENCHMARK_DEFINE_TEMPLATE_F(Redis, PipelineGrind)(benchmark::State& state) {
  RunStandalone([this, &state] {
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

    const auto stats = GetSentinel()->GetStatistics({});
    const auto total = stats.GetShardGroupTotalStatistics();
    const auto& timings = total.timings_percentile;
    for (auto p : {95, 99, 100}) {
      state.counters["p" + std::to_string(p)] = timings.GetPercentile(p);
    }
  });
}

BENCHMARK_INSTANTIATE_TEMPLATE_F(Redis, PipelineGrind, Ping)
    ->RangeMultiplier(2)
    ->Range(4, 32);

BENCHMARK_INSTANTIATE_TEMPLATE_F(Redis, PipelineGrind, Set)
    ->Args({16, 1024})
    ->Args({32, 1024});

}  // namespace storages::redis::bench

USERVER_NAMESPACE_END
