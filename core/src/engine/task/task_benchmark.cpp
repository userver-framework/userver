#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>

#include <engine/impl/standalone.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/single_threaded_task_processors_pool.hpp>
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

void engine_task_yield_single_thread(benchmark::State& state) {
  engine::RunStandalone([&] {
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(state.range(0) - 1);

    for (int i = 0; i < state.range(0) - 1; i++) {
      tasks.push_back(engine::AsyncNoSpan([]() {
        while (!engine::current_task::ShouldCancel()) engine::Yield();
      }));
    }

    for (auto _ : state) engine::Yield();
  });
}
BENCHMARK(engine_task_yield_single_thread)->RangeMultiplier(2)->Range(1, 128);

void engine_task_yield_multiple_threads(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<std::uint64_t>> tasks;
    tasks.reserve(state.range(0) - 1);

    for (int i = 0; i < state.range(0) - 1; i++) {
      tasks.push_back(engine::AsyncNoSpan([&] {
        std::uint64_t yields_performed = 0;
        while (keep_running) {
          engine::Yield();
          ++yields_performed;
        }
        return yields_performed;
      }));
    }

    std::uint64_t yields_performed = 0;
    for (auto _ : state) {
      engine::Yield();
      ++yields_performed;
    }

    keep_running = false;
    for (auto& task : tasks) {
      yields_performed += task.Get();
    }

    state.counters["yields"] =
        benchmark::Counter(yields_performed, benchmark::Counter::kIsRate);
    state.counters["yields/thread"] = benchmark::Counter(
        static_cast<double>(yields_performed) / state.range(0),
        benchmark::Counter::kIsRate);
  });
}
BENCHMARK(engine_task_yield_multiple_threads)
    ->RangeMultiplier(2)
    ->Range(1, 32)
    ->Arg(6)
    ->Arg(12);

void engine_task_yield_multiple_task_processors(benchmark::State& state) {
  engine::RunStandalone([&] {
    auto tp_pool = engine::SingleThreadedTaskProcessorsPool::MakeForTests(
        state.range(0) - 1);

    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<std::uint64_t>> tasks;
    tasks.reserve(state.range(0) - 1);

    for (int i = 0; i < state.range(0) - 1; i++) {
      tasks.push_back(engine::AsyncNoSpan(tp_pool.At(i), [&] {
        std::uint64_t yields_performed = 0;
        while (keep_running) {
          engine::Yield();
          ++yields_performed;
        }
        return yields_performed;
      }));
    }

    std::uint64_t yields_performed = 0;
    for (auto _ : state) {
      engine::Yield();
      ++yields_performed;
    }

    keep_running = false;
    for (auto& task : tasks) {
      yields_performed += task.Get();
    }

    state.counters["yields"] =
        benchmark::Counter(yields_performed, benchmark::Counter::kIsRate);
    state.counters["yields/thread"] = benchmark::Counter(
        static_cast<double>(yields_performed) / state.range(0),
        benchmark::Counter::kIsRate);
  });
}
BENCHMARK(engine_task_yield_multiple_task_processors)
    ->RangeMultiplier(2)
    ->Range(1, 32);

void thread_yield(benchmark::State& state) {
  for (auto _ : state) std::this_thread::yield();
}
BENCHMARK(thread_yield)->RangeMultiplier(2)->ThreadRange(1, 32);

USERVER_NAMESPACE_END
