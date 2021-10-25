#include <benchmark/benchmark.h>

#include <fmt/format.h>

#include <userver/concurrent/mutex_set.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

template <class T>
T GetKeyForBenchmark(int i) {
  if constexpr (std::is_same_v<std::string, T>) {
    return fmt::format("A string {}", i);
  } else {
    return T{} + i;
  }
}

template <class T>
void mutex_set_lock_unlock_no_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::MutexSet<T> ms;
    std::atomic<int> counter{0};

    const auto concurrent_jobs = state.range(0);
    std::vector<engine::Task> tasks;
    tasks.reserve(concurrent_jobs);

    tasks.push_back(engine::AsyncNoSpan([&]() {
      auto mutex = ms.GetMutexForKey(GetKeyForBenchmark<T>(++counter));

      for (auto _ : state) {
        std::unique_lock lock(mutex);
        benchmark::DoNotOptimize(lock);
      }
    }));

    for (auto& task : tasks) {
      task.Wait();
    }
  });
}

BENCHMARK_TEMPLATE(mutex_set_lock_unlock_no_contention, int)
    ->RangeMultiplier(2)
    ->Range(1, 8);
BENCHMARK_TEMPLATE(mutex_set_lock_unlock_no_contention, std::string)
    ->RangeMultiplier(2)
    ->Range(1, 8);

template <class T>
void mutex_set_lock_unlock_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::MutexSet<T> ms;

    const auto concurrent_jobs = state.range(0);
    std::vector<engine::Task> tasks;
    tasks.reserve(concurrent_jobs);

    for (int i = 0; i < concurrent_jobs; ++i) {
      tasks.push_back(engine::AsyncNoSpan([&]() {
        auto mutex0 = ms.GetMutexForKey(GetKeyForBenchmark<T>(0));
        auto mutex1 = ms.GetMutexForKey(GetKeyForBenchmark<T>(1));
        auto mutex2 = ms.GetMutexForKey(GetKeyForBenchmark<T>(2));
        auto mutex3 = ms.GetMutexForKey(GetKeyForBenchmark<T>(3));
        auto mutex4 = ms.GetMutexForKey(GetKeyForBenchmark<T>(4));

        for (auto _ : state) {
          std::unique_lock lock0(mutex0);
          std::unique_lock lock1(mutex1);
          std::unique_lock lock2(mutex2);
          std::unique_lock lock3(mutex3);
          std::unique_lock lock4(mutex4);

          benchmark::DoNotOptimize(lock0);
          benchmark::DoNotOptimize(lock1);
          benchmark::DoNotOptimize(lock2);
          benchmark::DoNotOptimize(lock3);
          benchmark::DoNotOptimize(lock4);
        }
      }));
    }

    for (auto& task : tasks) {
      task.Wait();
    }
  });
}

BENCHMARK_TEMPLATE(mutex_set_lock_unlock_contention, int)
    ->RangeMultiplier(2)
    ->Range(1, 8);
BENCHMARK_TEMPLATE(mutex_set_lock_unlock_contention, std::string)
    ->RangeMultiplier(2)
    ->Range(1, 8);

template <class T>
void mutex_set_8ways_lock_unlock_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::MutexSet<T> ms(8);

    const auto concurrent_jobs = state.range(0);
    std::vector<engine::Task> tasks;
    tasks.reserve(concurrent_jobs);

    for (int thread_no = 0; thread_no < concurrent_jobs; ++thread_no) {
      tasks.push_back(engine::AsyncNoSpan([&]() {
        constexpr std::size_t kKeysCount = 128;
        const auto addition = thread_no * kKeysCount;
        for (auto _ : state) {
          for (unsigned int i = 0; i < kKeysCount; ++i) {
            ms.GetMutexForKey(GetKeyForBenchmark<T>(addition + i)).lock();
          }
          for (unsigned int i = 0; i < kKeysCount; ++i) {
            ms.GetMutexForKey(GetKeyForBenchmark<T>(addition + i)).unlock();
          }
        }
      }));
    }

    for (auto& task : tasks) {
      task.Wait();
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
