#include <benchmark/benchmark.h>

#include <atomic>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/semaphore.hpp>
#include <userver/engine/sleep.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

/// [RunStandalone sample]
void semaphore_lock(benchmark::State& state) {
  engine::RunStandalone([&]() {
    std::size_t i = 0;
    engine::Semaphore sem{std::numeric_limits<std::size_t>::max()};

    for ([[maybe_unused]] auto _ : state) {
      sem.lock_shared();
      ++i;
    }

    for (std::size_t j = 0; j < i; ++j) {
      sem.unlock_shared();
    }
  });
}
BENCHMARK(semaphore_lock);
/// [RunStandalone sample]

void semaphore_unlock(benchmark::State& state) {
  engine::RunStandalone([&]() {
    unsigned i = 0;
    engine::Semaphore sem{std::numeric_limits<std::size_t>::max()};

    auto lock_multiple = [&i, &sem]() {
      for (; i < 1024; ++i) {
        sem.lock_shared();
      }
    };
    lock_multiple();

    for ([[maybe_unused]] auto _ : state) {
      sem.unlock_shared();

      if (--i == 0) {
        state.PauseTiming();
        lock_multiple();
        state.ResumeTiming();
      }
    }

    while (i) {
      sem.unlock_shared();
      --i;
    }
  });
}
BENCHMARK(semaphore_unlock);

void semaphore_lock_unlock_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    engine::Semaphore sem{1};

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        sem.lock_shared();
        sem.unlock_shared();
      }
    });
  });
}
BENCHMARK(semaphore_lock_unlock_contention)->RangeMultiplier(2)->Range(1, 32);

void semaphore_lock_unlock_payload_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    engine::Semaphore sem{1};

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        sem.lock_shared();
        {
          std::vector<int> tmp(32, 32);
          benchmark::DoNotOptimize(tmp);
        }
        sem.unlock_shared();
      }
    });
  });
}
BENCHMARK(semaphore_lock_unlock_payload_contention)
    ->RangeMultiplier(2)
    ->Range(1, 32);

void semaphore_lock_unlock_coro_contention(benchmark::State& state) {
  engine::RunStandalone(4, [&] {
    engine::Semaphore sem{1};

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        sem.lock_shared();
        sem.unlock_shared();
      }
    });
  });
}
BENCHMARK(semaphore_lock_unlock_coro_contention)
    ->RangeMultiplier(2)
    ->Range(1, 1024);

void semaphore_lock_unlock_payload_coro_contention(benchmark::State& state) {
  engine::RunStandalone(4, [&] {
    engine::Semaphore sem{1};

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        sem.lock_shared();
        {
          std::vector<int> tmp(32, 32);
          benchmark::DoNotOptimize(tmp);
        }
        sem.unlock_shared();
      }
    });
  });
}
BENCHMARK(semaphore_lock_unlock_payload_coro_contention)
    ->RangeMultiplier(2)
    ->Range(1, 1024);

void semaphore_lock_unlock_st_coro_contention(benchmark::State& state) {
  engine::RunStandalone([&]() {
    engine::Semaphore sem{1};

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        sem.lock_shared();
        engine::Yield();
        sem.unlock_shared();
      }
    });
  });
}
BENCHMARK(semaphore_lock_unlock_st_coro_contention)
    ->RangeMultiplier(2)
    ->Range(1, 1024);

USERVER_NAMESPACE_END
