#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>

#include <engine/async.hpp>
#include <engine/run_in_coro.hpp>
#include <engine/semaphore.hpp>
#include <engine/sleep.hpp>

#include <utils/gbench_auxilary.hpp>

void semaphore_lock(benchmark::State& state) {
  RunInCoro(
      [&]() {
        unsigned i = 0;
        engine::Semaphore sem{std::numeric_limits<std::size_t>::max()};

        for (auto _ : state) {
          sem.lock();
          ++i;
        }

        for (unsigned j = 0; j < i; ++j) {
          sem.unlock();
        }
      },
      1);
}
BENCHMARK(semaphore_lock);

void semaphore_unlock(benchmark::State& state) {
  RunInCoro(
      [&]() {
        unsigned i = 0;
        engine::Semaphore sem{std::numeric_limits<std::size_t>::max()};

        auto lock_multiple = [&i, &sem]() {
          for (; i < 1024; ++i) {
            sem.lock();
          }
        };
        lock_multiple();

        for (auto _ : state) {
          sem.unlock();

          if (--i == 0) {
            state.PauseTiming();
            lock_multiple();
            state.ResumeTiming();
          }
        }

        while (i) {
          sem.unlock();
          --i;
        }
      },
      1);
}
BENCHMARK(semaphore_unlock);

void semaphore_lock_unlock_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        engine::Semaphore sem{1};

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::impl::Async([&]() {
            while (run) {
              sem.lock();
              sem.unlock();
            }
          }));

        for (auto _ : state) {
          sem.lock();
          sem.unlock();
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK(semaphore_lock_unlock_contention)->RangeMultiplier(2)->Range(1, 32);

void semaphore_lock_unlock_payload_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        engine::Semaphore sem{1};

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::impl::Async([&]() {
            while (run) {
              sem.lock();
              {
                std::vector<int> tmp(32, 32);
                benchmark::DoNotOptimize(tmp);
              }
              sem.unlock();
            }
          }));

        for (auto _ : state) {
          sem.lock();
          {
            std::vector<int> tmp(32, 32);
            benchmark::DoNotOptimize(tmp);
          }
          sem.unlock();
        }

        run = false;
      },
      state.range(0));
}
BENCHMARK(semaphore_lock_unlock_payload_contention)
    ->RangeMultiplier(2)
    ->Range(1, 32);

void semaphore_lock_unlock_coro_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        engine::Semaphore sem{1};

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::impl::Async([&]() {
            while (run) {
              sem.lock();
              sem.unlock();
            }
          }));

        for (auto _ : state) {
          sem.lock();
          sem.unlock();
        }

        run = false;
      },
      4);
}
BENCHMARK(semaphore_lock_unlock_coro_contention)
    ->RangeMultiplier(2)
    ->Range(1, 1024);

void semaphore_lock_unlock_payload_coro_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        engine::Semaphore sem{1};

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::impl::Async([&]() {
            while (run) {
              sem.lock();
              {
                std::vector<int> tmp(32, 32);
                benchmark::DoNotOptimize(tmp);
              }
              sem.unlock();
            }
          }));

        for (auto _ : state) {
          sem.lock();
          {
            std::vector<int> tmp(32, 32);
            benchmark::DoNotOptimize(tmp);
          }
          sem.unlock();
        }

        run = false;
      },
      4);
}
BENCHMARK(semaphore_lock_unlock_payload_coro_contention)
    ->RangeMultiplier(2)
    ->Range(1, 1024);

void semaphore_lock_unlock_st_coro_contention(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::atomic<bool> run{true};
        engine::Semaphore sem{1};

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < state.range(0) - 1; i++)
          tasks.push_back(engine::impl::Async([&]() {
            while (run) {
              sem.lock();
              sem.unlock();
            }
          }));

        for (auto _ : state) {
          sem.lock();
          engine::Yield();
          sem.unlock();
        }

        run = false;
      },
      1);
}
BENCHMARK(semaphore_lock_unlock_st_coro_contention)
    ->RangeMultiplier(2)
    ->Range(1, 1024);
