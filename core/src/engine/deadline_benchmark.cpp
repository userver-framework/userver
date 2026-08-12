#include <benchmark/benchmark.h>

#include <chrono>

#include <userver/engine/deadline.hpp>

#include <utils/gbench_auxiliary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void DeadlineFromDuration(benchmark::State& state, std::chrono::nanoseconds duration) {
    for ([[maybe_unused]] auto _ : state) {
        auto deadline = engine::Deadline::FromDuration(duration);
        benchmark::DoNotOptimize(deadline);
    }
}

void DeadlineIsReached(benchmark::State& state, std::chrono::nanoseconds duration) {
    auto deadline = engine::Deadline::FromDuration(duration);
    for ([[maybe_unused]] auto _ : state) {
        bool is_reached = deadline.IsReached();
        benchmark::DoNotOptimize(is_reached);
    }
}

void Deadline1usIntervalConstruction(benchmark::State& state) {
    DeadlineFromDuration(state, std::chrono::microseconds{1});
}

void Deadline20msIntervalConstruction(benchmark::State& state) {
    DeadlineFromDuration(state, std::chrono::milliseconds{20});
}

void Deadline1usIntervalReached(benchmark::State& state) { DeadlineIsReached(state, std::chrono::microseconds{1}); }

void Deadline20msIntervalReached(benchmark::State& state) { DeadlineIsReached(state, std::chrono::milliseconds{20}); }

void Deadline100sIntervalReached(benchmark::State& state) { DeadlineIsReached(state, std::chrono::seconds{100}); }

}  // namespace

BENCHMARK(Deadline1usIntervalConstruction);
BENCHMARK(Deadline20msIntervalConstruction);

BENCHMARK(Deadline1usIntervalReached);
BENCHMARK(Deadline20msIntervalReached);
BENCHMARK(Deadline100sIntervalReached);

USERVER_NAMESPACE_END
