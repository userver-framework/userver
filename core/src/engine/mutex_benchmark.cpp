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
void GenericLock(benchmark::State& state) {
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
void GenericUnlock(benchmark::State& state) {
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
void GenericContention(benchmark::State& state) {
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
void GenericContentionWithPayload(benchmark::State& state) {
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

void MutexCoroLock(benchmark::State& state) {
    engine::RunStandalone([&] { GenericLock<engine::Mutex>(state); });
}

void MutexStdLock(benchmark::State& state) { GenericLock<std::mutex>(state); }

void SingleWaitingTaskMutexLock(benchmark::State& state) {
    engine::RunStandalone([&] { GenericLock<engine::SingleWaitingTaskMutex>(state); });
}

void MutexCoroUnlock(benchmark::State& state) {
    engine::RunStandalone([&] { GenericUnlock<engine::Mutex>(state); });
}

void MutexStdUnlock(benchmark::State& state) { GenericLock<std::mutex>(state); }

void SingleWaitingTaskMutexUnlock(benchmark::State& state) {
    engine::RunStandalone([&] { GenericUnlock<engine::SingleWaitingTaskMutex>(state); });
}

void MutexCoroContention(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] { GenericContention<engine::Mutex>(state); });
}

void MutexStdContention(benchmark::State& state) { GenericContention<std::mutex>(state); }

void SingleWaitingTaskMutexContention(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] { GenericContention<engine::SingleWaitingTaskMutex>(state); });
}

void MutexCoroContentionWithPayload(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] { GenericContentionWithPayload<engine::Mutex>(state); });
}

void MutexStdContentionWithPayload(benchmark::State& state) { GenericContentionWithPayload<std::mutex>(state); }

void SingleWaitingTaskMutexContentionWithPayload(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] { GenericContentionWithPayload<engine::SingleWaitingTaskMutex>(state); });
}

}  // namespace

BENCHMARK(MutexCoroLock);
BENCHMARK(MutexStdLock);
BENCHMARK(SingleWaitingTaskMutexLock);

BENCHMARK(MutexCoroUnlock);
BENCHMARK(MutexStdUnlock);
BENCHMARK(SingleWaitingTaskMutexUnlock);

BENCHMARK(MutexCoroContention)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(MutexStdContention)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(SingleWaitingTaskMutexContention)->Range(1, 2);

BENCHMARK(MutexCoroContentionWithPayload)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(MutexStdContentionWithPayload)->RangeMultiplier(2)->Range(1, 32);
BENCHMARK(SingleWaitingTaskMutexContentionWithPayload)->Range(1, 2);

USERVER_NAMESPACE_END
