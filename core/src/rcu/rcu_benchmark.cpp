#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <vector>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/async.hpp>

template <int VariableCount>
void rcu_read(benchmark::State& state) {
  engine::RunStandalone([&] {
    rcu::Variable<std::uint64_t> vars[VariableCount];
    {
      std::uint64_t i = 0;
      for (auto& var : vars) {
        var.Assign(i++);
      }
    }

    {
      std::uint64_t i = 0;
      for (auto _ : state) {
        auto reader = vars[i++ % VariableCount].Read();
        benchmark::DoNotOptimize(*reader);
      }
    }
  });
}
BENCHMARK_TEMPLATE(rcu_read, 1);
BENCHMARK_TEMPLATE(rcu_read, 2);
BENCHMARK_TEMPLATE(rcu_read, 4);

template <int VariableCount>
void rcu_write(benchmark::State& state) {
  engine::RunStandalone([&] {
    rcu::Variable<std::uint64_t> vars[VariableCount];

    std::uint64_t i = 0;
    for (auto _ : state) {
      vars[i % VariableCount].Assign(i);
      ++i;
    }
  });
}
BENCHMARK_TEMPLATE(rcu_write, 1);
BENCHMARK_TEMPLATE(rcu_write, 2);
BENCHMARK_TEMPLATE(rcu_write, 4);

template <int VariableCount>
void rcu_contention(benchmark::State& state) {
  const std::size_t readers_count = state.range(0);
  const std::size_t writers_count = state.range(1);

  engine::RunStandalone(readers_count + writers_count, [&] {
    std::atomic<bool> run{true};
    rcu::Variable<std::uint64_t> vars[VariableCount];

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(readers_count - 1 + writers_count);

    for (std::size_t i = 0; i < readers_count - 1; i++) {
      tasks.push_back(utils::Async("reader", [&, i]() {
        std::uint64_t j = i;
        while (run) {
          auto reader = vars[j++ % VariableCount].Read();
          benchmark::DoNotOptimize(*reader);
        }
      }));
    }

    for (std::size_t i = 0; i < writers_count; i++) {
      tasks.push_back(utils::Async("writer", [&]() {
        std::uint64_t i = 0;
        while (run) {
          vars[i % VariableCount].Assign(i);
          ++i;
        }
      }));
    }

    std::uint64_t i = 0;
    for (auto _ : state) {
      auto reader = vars[i++ % VariableCount].Read();
      benchmark::DoNotOptimize(*reader);
    }

    run = false;
    for (auto& task : tasks) {
      task.Get();
    }
  });
}
BENCHMARK_TEMPLATE(rcu_contention, 1)
    ->RangeMultiplier(2)
    ->Ranges({{1, 32}, {0, 1}});
BENCHMARK_TEMPLATE(rcu_contention, 2)
    ->RangeMultiplier(2)
    ->Ranges({{1, 32}, {0, 1}});
BENCHMARK_TEMPLATE(rcu_contention, 4)
    ->RangeMultiplier(2)
    ->Ranges({{1, 32}, {0, 1}});

void rcu_of_shared_ptr(benchmark::State& state) {
  const std::size_t readers_count = state.range(0);

  engine::RunStandalone(readers_count, [&] {
    std::atomic<bool> run{true};
    rcu::Variable<std::shared_ptr<std::uint64_t>> var;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(readers_count - 1);

    for (std::size_t i = 0; i < readers_count - 1; i++) {
      tasks.push_back(utils::Async("reader", [&] {
        while (run) {
          auto reader = var.ReadCopy();
          benchmark::DoNotOptimize(*reader);
        }
      }));
    }

    for (auto _ : state) {
      auto copy = var.ReadCopy();
      benchmark::DoNotOptimize(*copy);
    }

    run = false;
    for (auto& task : tasks) {
      task.Get();
    }
  });
}
BENCHMARK(rcu_of_shared_ptr)->RangeMultiplier(2)->Range(1, 32);
