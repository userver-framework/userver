#include <benchmark/benchmark.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <thread>

#include <concurrent/impl/latch.hpp>
#include <engine/impl/standalone.hpp>
#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>
#include <engine/task/work_stealing_queue/task_queue.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/single_threaded_task_processors_pool.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

void engine_task_create(benchmark::State& state) {
  // We use 2 threads to ensure that detached tasks are deallocated,
  // otherwise this benchmark OOMs after some time.
  engine::RunStandalone(2, [&] {
    for ([[maybe_unused]] auto _ : state) engine::AsyncNoSpan([]() {}).Detach();
  });
}
BENCHMARK(engine_task_create);

void engine_task_yield_single_thread(benchmark::State& state) {
  engine::RunStandalone([&] {
    RunParallelBenchmark(state, [](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        engine::Yield();
      }
    });
  });
}
BENCHMARK(engine_task_yield_single_thread)->RangeMultiplier(2)->Range(1, 128);

void engine_task_yield_multiple_threads(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::atomic<std::uint64_t> total_yields{0};

    RunParallelBenchmark(state, [&](auto& range) {
      std::uint64_t yields_performed = 0;
      for ([[maybe_unused]] auto _ : range) {
        engine::Yield();
        ++yields_performed;
      }
      total_yields += yields_performed;
    });

    state.counters["yields"] =
        benchmark::Counter(total_yields, benchmark::Counter::kIsRate);
    state.counters["yields/thread"] =
        benchmark::Counter(static_cast<double>(total_yields) / state.range(0),
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
    for ([[maybe_unused]] auto _ : state) {
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
  for ([[maybe_unused]] auto _ : state) std::this_thread::yield();
}
BENCHMARK(thread_yield)->RangeMultiplier(2)->ThreadRange(1, 32);

void engine_multiple_tasks_multiple_threads(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::atomic<std::uint64_t> tasks_count_total = 0;
    RunParallelBenchmark(state, [&](auto& range) {
      std::uint64_t tasks_count = 0;
      for ([[maybe_unused]] auto _ : range) {
        engine::AsyncNoSpan([] {}).Wait();
        tasks_count++;
      }
      tasks_count_total += tasks_count;
      benchmark::DoNotOptimize(tasks_count);
    });
    benchmark::DoNotOptimize(tasks_count_total);
  });
}
BENCHMARK(engine_multiple_tasks_multiple_threads)
    ->RangeMultiplier(2)
    ->Range(1, 32)
    ->Arg(6)
    ->Arg(12);

void engine_multiple_yield_two_task_processor_no_extra_wakeups(
    benchmark::State& state) {
  engine::RunStandalone([&] {
    std::vector<std::unique_ptr<engine::TaskProcessor>> processors;
    for (int i = 0; i < 2; i++) {
      engine::TaskProcessorConfig proc_config;
      proc_config.name = std::to_string(i);
      proc_config.thread_name = std::to_string(i);
      proc_config.worker_threads = state.range(0);
      processors.push_back(std::make_unique<engine::TaskProcessor>(
          std::move(proc_config),
          engine::current_task::GetTaskProcessor().GetTaskProcessorPools()));
    }
    auto tasks_count{state.range(0) / 2};
    std::vector<int64_t> tasks_per_tp{tasks_count, tasks_count - 1};
    std::vector<engine::TaskWithResult<std::uint64_t>> tasks;
    std::atomic<bool> keep_running{true};
    for (int i = 0; i < 2; i++) {
      for (auto j = 0; j < tasks_per_tp[i]; j++) {
        tasks.push_back(engine::AsyncNoSpan(*processors[i].get(), [&] {
          std::uint64_t yields_performed = 0;
          while (keep_running) {
            engine::Yield();
            ++yields_performed;
          }
          return yields_performed;
        }));
      }
    }

    tasks.push_back(engine::AsyncNoSpan(*processors.back().get(), [&] {
      std::uint64_t yields_performed = 0;
      for ([[maybe_unused]] auto _ : state) {
        engine::Yield();
        ++yields_performed;
      }
      keep_running = false;
      return yields_performed;
    }));

    std::uint64_t yields_performed = 0;
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
BENCHMARK(engine_multiple_yield_two_task_processor_no_extra_wakeups)
    ->RangeMultiplier(2)
    ->Range(2, 32)
    ->Arg(6)
    ->Arg(12);

void engine_tasks_from_another_task_processor(benchmark::State& state) {
  engine::RunStandalone([&] {
    engine::TaskProcessorConfig proc_config;
    proc_config.name = "benchmark";
    proc_config.thread_name = "benchmark";
    proc_config.worker_threads = state.range(0);
    engine::TaskProcessor task_processor(
        std::move(proc_config),
        engine::current_task::GetTaskProcessor().GetTaskProcessorPools());
    std::deque<engine::TaskWithResult<void>> tasks;
    for (std::size_t i = 0; i < proc_config.worker_threads; i++) {
      tasks.push_back(engine::AsyncNoSpan(task_processor, []() {}));
    }
    for ([[maybe_unused]] auto _ : state) {
      tasks.front().Wait();
      tasks.push_back(engine::AsyncNoSpan(task_processor, []() {}));
      tasks.pop_front();
    }
  });
}
BENCHMARK(engine_tasks_from_another_task_processor)
    ->RangeMultiplier(2)
    ->Range(2, 32)
    ->Arg(6)
    ->Arg(12);

USERVER_NAMESPACE_END
