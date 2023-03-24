#include <benchmark/benchmark.h>

#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

void engine_task_create(benchmark::State& state) {
  // We use 2 threads to ensure that detached tasks are deallocated,
  // otherwise this benchmark OOMs after some time.
  engine::RunStandalone(2, [&] {
    for (auto _ : state) engine::AsyncNoSpan([]() {}).Detach();
  });
}
BENCHMARK(engine_task_create);

void engine_task_yield(benchmark::State& state) {
  engine::RunStandalone([&] {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (int i = 0; i < state.range(0); i++)
      tasks.push_back(engine::AsyncNoSpan([]() {
        while (!engine::current_task::ShouldCancel()) engine::Yield();
      }));

    for (auto _ : state) engine::Yield();
  });
}
BENCHMARK(engine_task_yield)->Arg(0)->RangeMultiplier(2)->Range(1, 10);

void engine_task_yield_multiple_threads(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(state.range(0) - 1);
    for (int i = 0; i < state.range(0) - 1; i++)
      tasks.push_back(engine::AsyncNoSpan([]() {
        while (!engine::current_task::ShouldCancel()) engine::Yield();
      }));

    for (auto _ : state) engine::Yield();
  });
}
BENCHMARK(engine_task_yield_multiple_threads)->RangeMultiplier(2)->Range(1, 32);

void thread_yield(benchmark::State& state) {
  for (auto _ : state) std::this_thread::yield();
}
BENCHMARK(thread_yield)->RangeMultiplier(2)->ThreadRange(1, 32);

USERVER_NAMESPACE_END
