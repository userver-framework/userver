#include <benchmark/benchmark.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <random>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <utils/gbench_auxilary.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// Minimum offset between two objects to avoid false sharing
// TODO: replace with std::hardware_destructive_interference_size
constexpr std::size_t kInterferenceSize = 64;

class ThreadPool {
 public:
  template <class F>
  ThreadPool(unsigned count, F f) {
    for (unsigned i = 0; i < count; i++) {
      tasks.push_back(std::thread(f));
    }
  }

  void Wait() {
    for (auto& t : tasks) {
      t.join();
    }
  }

 private:
  std::vector<std::thread> tasks;
};

class AsyncCoroPool {
 public:
  template <class F>
  AsyncCoroPool(unsigned count, F f) {
    for (unsigned i = 0; i < count; i++) {
      tasks.push_back(engine::AsyncNoSpan(f));
    }
  }

  void Wait() {
    for (auto& t : tasks) {
      t.Wait();
    }
  }

 private:
  std::vector<engine::TaskWithResult<void>> tasks;
};

template <class T>
struct PoolForImpl {
  static_assert(sizeof(T) == 0, "No specialization for T");
};

template <>
struct PoolForImpl<std::mutex> {
  using Pool = ThreadPool;
};

template <>
struct PoolForImpl<engine::Mutex> {
  using Pool = AsyncCoroPool;
};

template <class T>
using PoolFor = typename PoolForImpl<T>::Pool;

//////// Generic cases for benchmarking

template <class Mutex>
void generic_lock(benchmark::State& state) {
  unsigned i = 0;
  constexpr unsigned mutex_count = 256;
  Mutex mutexes[mutex_count];

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
}

template <class Mutex>
void generic_unlock(benchmark::State& state) {
  unsigned i = 0;
  constexpr unsigned mutex_count = 256;
  Mutex mutexes[mutex_count];

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
}

template <class Mutex>
void generic_contention(benchmark::State& state) {
  alignas(kInterferenceSize) std::atomic<bool> run{true};
  alignas(kInterferenceSize) std::atomic<std::size_t> lock_unlock_count{0};
  alignas(kInterferenceSize) Mutex m;

  PoolFor<Mutex> pool(state.range(0) - 1, [&]() {
    while (run) {
      m.lock();
      m.unlock();
      ++lock_unlock_count;
    }
  });

  for (auto _ : state) {
    m.lock();
    m.unlock();
    ++lock_unlock_count;
  }

  run = false;
  pool.Wait();
  state.SetItemsProcessed(lock_unlock_count.load());
}

template <class Mutex>
void generic_contention_with_payload(benchmark::State& state) {
  alignas(kInterferenceSize) std::atomic<bool> run{true};
  alignas(kInterferenceSize) std::atomic<std::size_t> lock_unlock_count{0};
  alignas(kInterferenceSize) Mutex m;
  alignas(kInterferenceSize) std::atomic<std::size_t> total_result{0};
  std::mt19937_64 engine;

  PoolFor<Mutex> pool(state.range(0) - 1, [&]() {
    std::size_t result{0};
    while (run) {
      m.lock();
      result += engine();
      m.unlock();
      ++lock_unlock_count;
    }
    total_result += result;
  });

  std::size_t result{0};
  for (auto _ : state) {
    m.lock();
    result += engine();
    m.unlock();
    ++lock_unlock_count;
  }
  total_result += result;

  run = false;
  pool.Wait();
  state.SetItemsProcessed(lock_unlock_count.load());
  benchmark::DoNotOptimize(total_result.load());
}

//////// Benchmarks

// Note: We intentionally do not run std::* benchmarks from RunStandalone to
// avoid any side-effects (RunStandalone spawns additional std::threads and uses
// some synchronization primitives).

void mutex_coro_lock(benchmark::State& state) {
  engine::RunStandalone([&] { generic_lock<engine::Mutex>(state); });
}

void mutex_std_lock(benchmark::State& state) {
  generic_lock<std::mutex>(state);
}

void mutex_coro_unlock(benchmark::State& state) {
  engine::RunStandalone([&] { generic_unlock<engine::Mutex>(state); });
}

void mutex_std_unlock(benchmark::State& state) {
  generic_lock<std::mutex>(state);
}

void mutex_coro_contention(benchmark::State& state) {
  engine::RunStandalone(state.range(0),
                        [&] { generic_contention<engine::Mutex>(state); });
}

void mutex_std_contention(benchmark::State& state) {
  generic_contention<std::mutex>(state);
}

void mutex_coro_contention_with_payload(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    generic_contention_with_payload<engine::Mutex>(state);
  });
}

void mutex_std_contention_with_payload(benchmark::State& state) {
  generic_contention_with_payload<std::mutex>(state);
}

}  // namespace

BENCHMARK(mutex_coro_lock);
BENCHMARK(mutex_std_lock);

BENCHMARK(mutex_coro_unlock);
BENCHMARK(mutex_std_unlock);

BENCHMARK(mutex_coro_contention)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(mutex_std_contention)->RangeMultiplier(2)->Range(1, 32);

BENCHMARK(mutex_coro_contention_with_payload)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(mutex_std_contention_with_payload)->RangeMultiplier(2)->Range(1, 32);

USERVER_NAMESPACE_END
