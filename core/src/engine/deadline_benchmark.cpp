#include <benchmark/benchmark.h>

#include <chrono>

#include <userver/engine/deadline.hpp>

#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void deadline_from_duration(benchmark::State& state,
                            std::chrono::nanoseconds duration) {
  for (auto _ : state) {
    auto deadline = engine::Deadline::FromDuration(duration);
    benchmark::DoNotOptimize(deadline);
  }
}

void deadline_is_reached(benchmark::State& state,
                         std::chrono::nanoseconds duration) {
  auto deadline = engine::Deadline::FromDuration(duration);
  for (auto _ : state) {
    bool is_reached = deadline.IsReached();
    benchmark::DoNotOptimize(is_reached);
  }
}

void deadline_1us_interval_construction(benchmark::State& state) {
  deadline_from_duration(state, std::chrono::microseconds{1});
}

void deadline_20ms_interval_construction(benchmark::State& state) {
  deadline_from_duration(state, std::chrono::milliseconds{20});
}

void deadline_1us_interval_reached(benchmark::State& state) {
  deadline_is_reached(state, std::chrono::microseconds{1});
}

void deadline_20ms_interval_reached(benchmark::State& state) {
  deadline_is_reached(state, std::chrono::milliseconds{20});
}

void deadline_100s_interval_reached(benchmark::State& state) {
  deadline_is_reached(state, std::chrono::seconds{100});
}

}  // namespace

BENCHMARK(deadline_1us_interval_construction);
BENCHMARK(deadline_20ms_interval_construction);

BENCHMARK(deadline_1us_interval_reached);
BENCHMARK(deadline_20ms_interval_reached);
BENCHMARK(deadline_100s_interval_reached);

USERVER_NAMESPACE_END
