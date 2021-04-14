#include <benchmark/benchmark.h>

#include <unordered_map>

#include <engine/run_in_coro.hpp>
#include <rcu/rcu.hpp>

#include <engine/async.hpp>
#include <engine/mutex.hpp>
#include <engine/run_in_coro.hpp>
#include <engine/sleep.hpp>
#include <utils/gbench_auxilary.hpp>
#include <utils/swappingsmart.hpp>

template <int VariableCount>
void rcu_read(benchmark::State& state) {
  RunInCoro([&]() {
    rcu::Variable<int> ptrs[VariableCount];
    {
      int i = 0;
      for (auto& ptr : ptrs) {
        ptr.Assign(i++);
      }
    }

    {
      int i = 0;
      for (auto _ : state) {
        auto rcu_ptr = ptrs[i++ % VariableCount].Read();
        benchmark::DoNotOptimize(*rcu_ptr);
      }
    }
  });
}
BENCHMARK_TEMPLATE(rcu_read, 1);
BENCHMARK_TEMPLATE(rcu_read, 2);
BENCHMARK_TEMPLATE(rcu_read, 4);

using std::literals::chrono_literals::operator""ms;

template <int VariableCount>
void rcu_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        rcu::Variable<std::unordered_map<int, int>> ptrs[VariableCount];

        std::vector<engine::TaskWithResult<void>> tasks;
        tasks.reserve(state.range(0) - 2);
        for (int i = 0; i < state.range(0) - 2; i++) {
          tasks.push_back(engine::impl::Async([&, i]() {
            int j = i;
            while (run) {
              auto rcu_ptr = ptrs[j++ % VariableCount].Read();
              benchmark::DoNotOptimize(*rcu_ptr);
            }
          }));
        }

        if (state.range(1)) {
          tasks.push_back(engine::impl::Async([&]() {
            int i = 0;
            while (run) {
              auto writer = ptrs[i % VariableCount].StartWrite();
              (*writer)[1] = i++;
              writer.Commit();
              engine::SleepFor(10ms);
            }
          }));
        }

        int i = 0;
        for (auto _ : state) {
          auto rcu_ptr = ptrs[i++ % VariableCount].Read();
          benchmark::DoNotOptimize(*rcu_ptr);
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK_TEMPLATE(rcu_contention, 1)
    ->RangeMultiplier(2)
    ->Ranges({{2, 32}, {false, true}});
BENCHMARK_TEMPLATE(rcu_contention, 2)
    ->RangeMultiplier(2)
    ->Ranges({{2, 32}, {false, true}});
BENCHMARK_TEMPLATE(rcu_contention, 4)
    ->RangeMultiplier(2)
    ->Ranges({{2, 32}, {false, true}});
