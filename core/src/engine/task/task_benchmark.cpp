#include <benchmark/benchmark.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <engine/async.hpp>
#include <engine/run_in_coro.hpp>
#include <engine/sleep.hpp>
#include <engine/standalone.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task.hpp>
#include <utils/gbench_auxilary.hpp>

void engine_task_create(benchmark::State& state) {
  RunInCoro(
      [&]() {
        for (auto _ : state) engine::Async([]() {}).Detach();
      },
      1);
}
BENCHMARK(engine_task_create);

void engine_task_yield(benchmark::State& state) {
  RunInCoro(
      [&]() {

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0); i++)
          tasks.push_back(engine::Async([]() {
            while (!engine::current_task::ShouldCancel()) engine::Yield();
          }));

        for (auto _ : state) engine::Yield();
      },
      1);
}
BENCHMARK(engine_task_yield)->Arg(0)->RangeMultiplier(2)->Range(1, 10);

void engine_task_yield_multiple_threads(benchmark::State& state) {
  RunInCoro(
      [&]() {

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::Async([]() {
            while (!engine::current_task::ShouldCancel()) engine::Yield();
          }));

        for (auto _ : state) engine::Yield();
      },
      state.range(0));
}
BENCHMARK(engine_task_yield_multiple_threads)->RangeMultiplier(2)->Range(1, 32);

void thread_yield(benchmark::State& state) {
  for (auto _ : state) std::this_thread::yield();
}
BENCHMARK(thread_yield)->RangeMultiplier(2)->ThreadRange(1, 32);
