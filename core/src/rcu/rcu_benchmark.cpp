#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <queue>
#include <vector>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

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

void rcu_contention(benchmark::State& state) {
  const std::size_t readers_count = state.range(0);
  const std::size_t writers_count = state.range(1);
  const std::size_t kept_readable_pointers_count = state.range(2);

  const std::size_t thread_count =
      std::min(readers_count + writers_count, std::size_t{6});

  engine::RunStandalone(thread_count, [&] {
    std::atomic<bool> run{true};
    rcu::Variable<std::uint64_t> var{0};

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(readers_count - 1 + writers_count);

    for (std::size_t j = 0; j < readers_count - 1; j++) {
      tasks.push_back(utils::Async("reader", [&] {
        std::vector<rcu::ReadablePtr<std::uint64_t>> pointers;
        pointers.reserve(kept_readable_pointers_count);

        while (run) {
          for (std::size_t i = 0; i < kept_readable_pointers_count; i++) {
            pointers.push_back(var.Read());
            benchmark::DoNotOptimize(*pointers.back());
          }
          engine::Yield();
          pointers.clear();
        }
      }));
    }

    for (std::size_t i = 0; i < writers_count; i++) {
      tasks.push_back(utils::Async("writer", [&]() {
        std::uint64_t i = 0;
        while (run) {
          var.Assign(++i);
        }
      }));
    }

    {
      std::queue<rcu::ReadablePtr<std::uint64_t>> pointers;
      for (std::size_t i = 0; i < kept_readable_pointers_count; i++) {
        pointers.push(var.Read());
      }

      for (auto _ : state) {
        pointers.pop();
        pointers.push(var.Read());
        benchmark::DoNotOptimize(*pointers.back());
      }
    }

    run = false;
    for (auto& task : tasks) {
      task.Get();
    }
  });
}
BENCHMARK(rcu_contention)
    ->RangeMultiplier(2)
    ->Ranges({{1, 16}, {0, 1}, {1, 4}})
    ->Ranges({{2048, 2048}, {0, 1}, {1, 4}});

void rcu_of_shared_ptr(benchmark::State& state) {
  const std::size_t readers_count = state.range(0);

  engine::RunStandalone(readers_count, [&] {
    std::atomic<bool> run{true};
    rcu::Variable<std::shared_ptr<std::uint64_t>> var{
        std::make_shared<std::uint64_t>(42)};

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

USERVER_NAMESPACE_END
