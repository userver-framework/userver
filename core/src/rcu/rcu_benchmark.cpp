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

void rcu_read(benchmark::State& state) {
  RunInCoro(
      [&]() {
        rcu::Variable<int> ptr(std::make_unique<int>(1));

        for (auto _ : state) {
          benchmark::DoNotOptimize(*ptr.Read());
        }
      },
      1);
}
BENCHMARK(rcu_read);

using std::literals::chrono_literals::operator""ms;

void rcu_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        rcu::Variable<std::unordered_map<int, int>> ptr;

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 2; i++)
          tasks.push_back(engine::impl::Async([&]() {
            while (run) {
              benchmark::DoNotOptimize(*ptr.Read());
            }
          }));

        if (state.range(1))
          tasks.push_back(engine::impl::Async([&]() {
            size_t i = 0;
            while (run) {
              auto writer = ptr.StartWrite();
              (*writer)[1] = i++;
              writer.Commit();
              engine::SleepFor(10ms);
            }
          }));

        for (auto _ : state) {
          benchmark::DoNotOptimize(*ptr.Read());
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK(rcu_contention)->RangeMultiplier(2)->Ranges({{2, 32}, {true, false}});
