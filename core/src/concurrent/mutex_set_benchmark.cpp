#include <userver/concurrent/mutex_set.hpp>

#include <array>
#include <atomic>
#include <string>
#include <type_traits>
#include <vector>

#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <typename T>
T GetKeyForBenchmark(std::size_t i) {
  if constexpr (std::is_same_v<std::string, T>) {
    return fmt::format("A string {}", i);
  } else {
    return T{} + i;
  }
}

template <typename T>
void mutex_set_lock_unlock_no_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::MutexSet<T> ms;

    const std::size_t concurrent_jobs = state.range(0);
    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(concurrent_jobs);

    for (std::size_t thread_id = 1; thread_id < concurrent_jobs; ++thread_id) {
      tasks.push_back(engine::AsyncNoSpan([&, thread_id] {
        auto mutex = ms.GetMutexForKey(GetKeyForBenchmark<T>(thread_id));

        while (keep_running) {
          std::unique_lock lock(mutex);
          benchmark::DoNotOptimize(lock);
        }
      }));
    }

    {
      auto mutex = ms.GetMutexForKey(GetKeyForBenchmark<T>(0));

      for (auto _ : state) {
        std::unique_lock lock(mutex);
        benchmark::DoNotOptimize(lock);
      }
    }

    keep_running = false;

    for (auto& task : tasks) {
      task.Get();
    }
  });
}

BENCHMARK_TEMPLATE(mutex_set_lock_unlock_no_contention, int)
    ->RangeMultiplier(2)
    ->Range(1, 8);
BENCHMARK_TEMPLATE(mutex_set_lock_unlock_no_contention, std::string)
    ->RangeMultiplier(2)
    ->Range(1, 8);

template <typename T>
void mutex_set_lock_unlock_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::MutexSet<T> ms;

    using Mutexes = std::array<concurrent::ItemMutex<T, std::equal_to<T>>, 5>;

    const auto make_mutexes = [&] {
      return Mutexes{{
          ms.GetMutexForKey(GetKeyForBenchmark<T>(0)),
          ms.GetMutexForKey(GetKeyForBenchmark<T>(1)),
          ms.GetMutexForKey(GetKeyForBenchmark<T>(2)),
          ms.GetMutexForKey(GetKeyForBenchmark<T>(3)),
          ms.GetMutexForKey(GetKeyForBenchmark<T>(4)),
      }};
    };

    const auto do_work = [&](Mutexes& mutexes) {
      std::unique_lock lock0(mutexes[0]);
      std::unique_lock lock1(mutexes[1]);
      std::unique_lock lock2(mutexes[2]);
      std::unique_lock lock3(mutexes[3]);
      std::unique_lock lock4(mutexes[4]);

      benchmark::DoNotOptimize(lock0);
      benchmark::DoNotOptimize(lock1);
      benchmark::DoNotOptimize(lock2);
      benchmark::DoNotOptimize(lock3);
      benchmark::DoNotOptimize(lock4);
    };

    const std::size_t concurrent_jobs = state.range(0);
    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(concurrent_jobs);

    for (std::size_t thread_id = 1; thread_id < concurrent_jobs; ++thread_id) {
      tasks.push_back(engine::AsyncNoSpan([&] {
        auto mutexes = make_mutexes();
        while (keep_running) {
          do_work(mutexes);
        }
      }));
    }

    auto mutexes = make_mutexes();
    for (auto _ : state) {
      do_work(mutexes);
    }

    keep_running = false;

    for (auto& task : tasks) {
      task.Get();
    }
  });
}

BENCHMARK_TEMPLATE(mutex_set_lock_unlock_contention, int)
    ->RangeMultiplier(2)
    ->Range(1, 8);
BENCHMARK_TEMPLATE(mutex_set_lock_unlock_contention, std::string)
    ->RangeMultiplier(2)
    ->Range(1, 8);

template <typename T>
void mutex_set_8ways_lock_unlock_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    constexpr std::size_t kKeysCount = 128;
    concurrent::MutexSet<T> ms(8);

    const auto do_work = [&](std::size_t thread_id) {
      const std::size_t addition = thread_id * kKeysCount;

      for (std::size_t i = 0; i < kKeysCount; ++i) {
        ms.GetMutexForKey(GetKeyForBenchmark<T>(addition + i)).lock();
      }
      for (std::size_t i = 0; i < kKeysCount; ++i) {
        ms.GetMutexForKey(GetKeyForBenchmark<T>(addition + i)).unlock();
      }
    };

    const std::size_t concurrent_jobs = state.range(0);
    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(concurrent_jobs);

    for (std::size_t thread_id = 1; thread_id < concurrent_jobs; ++thread_id) {
      tasks.push_back(engine::AsyncNoSpan([&, thread_id] {
        while (keep_running) {
          do_work(thread_id);
        }
      }));
    }

    for (auto _ : state) {
      do_work(0);
    }

    keep_running = false;

    for (auto& task : tasks) {
      task.Get();
    }
  });
}

BENCHMARK_TEMPLATE(mutex_set_8ways_lock_unlock_contention, int)
    ->RangeMultiplier(2)
    ->Range(1, 8);
BENCHMARK_TEMPLATE(mutex_set_8ways_lock_unlock_contention, std::string)
    ->RangeMultiplier(2)
    ->Range(1, 8);

}  // namespace

USERVER_NAMESPACE_END
