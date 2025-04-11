#include <benchmark/benchmark.h>

#include <atomic>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_waiting_task_mutex.hpp>
#include <userver/utils/rand.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

//////// Generic cases for benchmarking

template <typename Mutex>
void generic_lock(benchmark::State& state) {
    constexpr std::size_t kMutexCount = 256;
    std::size_t i = 0;
    Mutex mutexes[kMutexCount];

    for ([[maybe_unused]] auto _ : state) {
        mutexes[i].lock();

        if (++i == kMutexCount) {
            state.PauseTiming();
            i = 0;
            for (auto& m : mutexes) {
                m.unlock();
            }
            state.ResumeTiming();
        }
    }

    for (std::size_t j = 0; j < i; ++j) {
        mutexes[j].unlock();
    }
}

template <typename Mutex>
void generic_unlock(benchmark::State& state) {
    constexpr std::size_t kMutexCount = 256;
    std::size_t i = 0;
    Mutex mutexes[kMutexCount];

    for (auto& m : mutexes) {
        m.lock();
    }

    for ([[maybe_unused]] auto _ : state) {
        mutexes[i].unlock();

        if (++i == kMutexCount) {
            state.PauseTiming();
            i = 0;
            for (auto& m : mutexes) {
                m.lock();
            }
            state.ResumeTiming();
        }
    }

    for (; i < kMutexCount; ++i) {
        mutexes[i].unlock();
    }
}

template <typename Mutex>
void generic_contention(benchmark::State& state) {
    std::atomic<std::size_t> lock_unlock_count{0};
    concurrent::impl::InterferenceShield<Mutex> m;

    RunParallelBenchmark(state, [&](auto& range) {
        std::uint64_t local_lock_unlock_count = 0;

        for ([[maybe_unused]] auto _ : range) {
            m->lock();
            m->unlock();
            ++local_lock_unlock_count;
        }

        lock_unlock_count += local_lock_unlock_count;
    });

    const auto total_lock_unlock_count = static_cast<double>(lock_unlock_count.load());
    state.counters["locks"] = benchmark::Counter(total_lock_unlock_count, benchmark::Counter::kIsRate);
    state.counters["locks-per-thread"] =
        benchmark::Counter(total_lock_unlock_count / state.range(0), benchmark::Counter::kIsRate);
}

template <typename Mutex>
void generic_contention_with_payload(benchmark::State& state) {
    std::atomic<std::uint64_t> lock_unlock_count{0};
    concurrent::impl::InterferenceShield<Mutex> m;

    RunParallelBenchmark(state, [&](auto& range) {
        std::uint64_t local_lock_unlock_count = 0;

        for ([[maybe_unused]] auto _ : range) {
            m->lock();
            for (int i = 0; i < 10; ++i) {
                benchmark::DoNotOptimize(utils::Rand());
            }
            m->unlock();
            ++local_lock_unlock_count;
        }

        lock_unlock_count += local_lock_unlock_count;
    });

    const auto total_lock_unlock_count = static_cast<double>(lock_unlock_count.load());
    state.counters["locks"] = benchmark::Counter(total_lock_unlock_count, benchmark::Counter::kIsRate);
    state.counters["locks-per-thread"] =
        benchmark::Counter(total_lock_unlock_count / state.range(0), benchmark::Counter::kIsRate);
}

//////// Benchmarks

// Note: We intentionally do not run std::* benchmarks from RunStandalone to
// avoid any side-effects (RunStandalone spawns additional std::threads and uses
// some synchronization primitives).

void mutex_coro_lock(benchmark::State& state) {
    engine::RunStandalone([&] { generic_lock<engine::Mutex>(state); });
}

void mutex_std_lock(benchmark::State& state) { generic_lock<std::mutex>(state); }

void single_waiting_task_mutex_lock(benchmark::State& state) {
    engine::RunStandalone([&] { generic_lock<engine::SingleWaitingTaskMutex>(state); });
}

void mutex_coro_unlock(benchmark::State& state) {
    engine::RunStandalone([&] { generic_unlock<engine::Mutex>(state); });
}

void mutex_std_unlock(benchmark::State& state) { generic_lock<std::mutex>(state); }

void single_waiting_task_mutex_unlock(benchmark::State& state) {
    engine::RunStandalone([&] { generic_unlock<engine::SingleWaitingTaskMutex>(state); });
}

void mutex_coro_contention(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] { generic_contention<engine::Mutex>(state); });
}

void mutex_std_contention(benchmark::State& state) { generic_contention<std::mutex>(state); }

void single_waiting_task_mutex_contention(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] { generic_contention<engine::SingleWaitingTaskMutex>(state); });
}

void mutex_coro_contention_with_payload(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] { generic_contention_with_payload<engine::Mutex>(state); });
}

void mutex_std_contention_with_payload(benchmark::State& state) { generic_contention_with_payload<std::mutex>(state); }

void single_waiting_task_mutex_contention_with_payload(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] {
        generic_contention_with_payload<engine::SingleWaitingTaskMutex>(state);
    });
}

}  // namespace

BENCHMARK(mutex_coro_lock);
BENCHMARK(mutex_std_lock);
BENCHMARK(single_waiting_task_mutex_lock);

BENCHMARK(mutex_coro_unlock);
BENCHMARK(mutex_std_unlock);
BENCHMARK(single_waiting_task_mutex_unlock);

BENCHMARK(mutex_coro_contention)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(mutex_std_contention)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(single_waiting_task_mutex_contention)->Range(1, 2);

BENCHMARK(mutex_coro_contention_with_payload)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(mutex_std_contention_with_payload)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(single_waiting_task_mutex_contention_with_payload)->Range(1, 2);

USERVER_NAMESPACE_END
