#include <benchmark/benchmark.h>
#include <moodycamel/concurrentqueue.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

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
#include <concurrent/impl/latch.hpp>

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
      for ([[maybe_unused]] auto _ : tasks_per_tp) {
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

boost::intrusive_ptr<engine::impl::TaskContext> MakeContext() {
  return engine::impl::MakeTask({engine::current_task::GetTaskProcessor(),
                                 engine::Task::Importance::kNormal,
                                 engine::Task::WaitMode::kSingleWaiter,
                                 {}},
                                [] {})
      .Extract();
}

auto MakeContexts(const std::size_t tasks_count) {
  std::vector<boost::intrusive_ptr<engine::impl::TaskContext>> contexts;
  contexts.reserve(tasks_count);
  for (std::size_t i = 0; i < tasks_count; ++i) {
    contexts.push_back(MakeContext());
  }

  return contexts;
}

template <typename QueueType>
void engine_task_queue(
    benchmark::State& state) {
  engine::TaskProcessorConfig config;
  config.worker_threads = state.range(0);
  QueueType task_queue(config);
  std::atomic<bool> keep_running{true};
  std::atomic<std::size_t> push_pop_count = 0;
  concurrent::impl::Latch workers_left{
        static_cast<std::ptrdiff_t>(config.worker_threads)};
  
  auto step = [&task_queue](std::vector<boost::intrusive_ptr<engine::impl::TaskContext>>& tasks) {
    const std::size_t tasks_count = tasks.size();
    for (auto&& task : tasks) {
      task_queue.Push(std::move(task));
    }

    tasks.clear();
    for (std::size_t i = 0; i < tasks_count; i++) {
      auto context = task_queue.PopBlocking();
      if (!context) {
        break;
      }
      tasks.push_back(context);
    }
  };
  auto common_task = [&](std::vector<boost::intrusive_ptr<engine::impl::TaskContext>> tasks) {
    std::size_t iter_count = 0;
    while (keep_running) {
      step(tasks);
      ++iter_count;
    }
    push_pop_count += iter_count;
    workers_left.count_down();
  };

  auto state_task = [&](std::vector<boost::intrusive_ptr<engine::impl::TaskContext>> tasks) {
    std::size_t iter_count = 0;
    for ([[maybe_unused]] auto _ : state) {
      step(tasks);
      ++iter_count;
    }
    keep_running = false;
    push_pop_count += iter_count;
    workers_left.count_down();
  };
  
  engine::RunStandalone([&] {
    std::vector<std::thread> competing_threads;
    benchmark::DoNotOptimize(push_pop_count);
    for (int64_t i = 0; i < state.range(0) - 1; i++) {
      auto tasks = MakeContexts(1);
      competing_threads.emplace_back(common_task, tasks);
    }
    auto tasks = MakeContexts(1);
    competing_threads.emplace_back( state_task, tasks);
    workers_left.wait();
    for (auto& thread : competing_threads) {
      thread.join();
    }
    state.counters["push_pop"] =
        benchmark::Counter(push_pop_count, benchmark::Counter::kIsRate);
    state.counters["push_pop/thread"] = benchmark::Counter(
        static_cast<double>(push_pop_count) / state.range(0),
        benchmark::Counter::kIsRate);
  });
}
BENCHMARK_TEMPLATE(engine_task_queue, engine::WorkStealingTaskQueue)
    ->RangeMultiplier(2)
    ->Ranges({{1, 32}, {1, 2}})
    ->Args({6, 1})
    ->Args({6, 2})
    ->Args({12, 1})
    ->Args({12, 2});
BENCHMARK_TEMPLATE(engine_task_queue, engine::TaskQueue)
    ->Ranges({{1, 32}, {1, 2}})
    ->Args({6, 1})
    ->Args({6, 2})
    ->Args({12, 1})
    ->Args({12, 2});


void moody_camel(
    benchmark::State& state) {
  moodycamel::ConcurrentQueue<std::size_t> task_queue;
  std::atomic<bool> keep_running{true};
  std::atomic<int> push_pop_count = 0;
  std::atomic<std::size_t> size{1};
  
  auto step = [&task_queue, &size](std::size_t tasks_count, moodycamel::ConsumerToken& token) {
    for (std::size_t i = 0; i < tasks_count; i++) {
      task_queue.enqueue(i);
    }
    for (std::size_t i = 0; i < tasks_count;) {
      std::size_t x{0};
      if (task_queue.try_dequeue(token, x)) {
        i++;
      }
      if (size.load() == 0) {
        return;
      }
    }
  };
  auto common_task = [&]() {
    moodycamel::ConsumerToken token(task_queue);
    std::size_t iter_count = 0;
    while (keep_running) {
      step(1, token);
      ++iter_count;
    }
    push_pop_count += iter_count;
  };

  auto state_task = [&]() {
    moodycamel::ConsumerToken token(task_queue);
    std::size_t iter_count = 0;
    for ([[maybe_unused]] auto _ : state) {
      step(1, token);
      ++iter_count;
    }
    keep_running = false;
    push_pop_count += iter_count;
    size--;
  };
  
  engine::RunStandalone([&] {
    std::vector<std::thread> competing_threads;
    benchmark::DoNotOptimize(push_pop_count);
    for (int64_t i = 0; i < state.range(0) - 1; i++) {
      competing_threads.emplace_back(common_task);
    }
    competing_threads.emplace_back( state_task);
    for (auto& thread : competing_threads) {
      thread.join();
    }
    state.counters["push_pop"] =
        benchmark::Counter(push_pop_count, benchmark::Counter::kIsRate);
    state.counters["push_pop/thread"] = benchmark::Counter(
        static_cast<double>(push_pop_count) / state.range(0),
        benchmark::Counter::kIsRate);
  });
}
BENCHMARK(moody_camel)
    ->RangeMultiplier(2)
    ->Range(1, 32);

USERVER_NAMESPACE_END
