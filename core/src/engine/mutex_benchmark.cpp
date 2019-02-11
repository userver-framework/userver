#include <benchmark/benchmark.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <engine/async.hpp>
#include <engine/mutex.hpp>
#include <engine/run_in_coro.hpp>
#include <engine/sleep.hpp>
#include <utils/gbench_auxilary.hpp>

void mutex_lock(benchmark::State& state) {
  RunInCoro(
      [&]() {
        unsigned i = 0;
        constexpr unsigned mutex_count = 256;
        engine::Mutex mutexes[mutex_count];

        for (auto _ : state) {
          mutexes[i].lock();

          if (++i == mutex_count) {
            state.PauseTiming();
            i = 0;
            for (auto& m : mutexes) {
              m.unlock();
            }
            state.ResumeTiming();
          }
        }

        for (unsigned j = 0; j < i; ++j) {
          mutexes[j].unlock();
        }
      },
      1);
}
BENCHMARK(mutex_lock);

void mutex_unlock(benchmark::State& state) {
  RunInCoro(
      [&]() {
        unsigned i = 0;
        constexpr unsigned mutex_count = 256;
        engine::Mutex mutexes[mutex_count];

        for (auto& m : mutexes) {
          m.lock();
        }

        for (auto _ : state) {
          mutexes[i].unlock();

          if (++i == mutex_count) {
            state.PauseTiming();
            i = 0;
            for (auto& m : mutexes) {
              m.lock();
            }
            state.ResumeTiming();
          }
        }

        for (; i < mutex_count; ++i) {
          mutexes[i].unlock();
        }
      },
      1);
}
BENCHMARK(mutex_unlock);

void mutex_lock_unlock_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        engine::Mutex m;

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::Async([&]() {
            while (run) {
              m.lock();
              m.unlock();
            }
          }));

        for (auto _ : state) {
          m.lock();
          m.unlock();
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK(mutex_lock_unlock_contention)->RangeMultiplier(2)->Range(1, 32);
