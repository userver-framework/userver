#include <userver/concurrent/impl/monotonic_concurrent_set.hpp>

#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <benchmark/benchmark.h>
#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/rcu/rcu_map.hpp>

#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct TestItem {
    int key;
    std::string value;

    TestItem(int k, std::string v)
        : key(k),
          value(std::move(v))
    {}

    bool operator==(const TestItem& other) const { return key == other.key; }
};

struct TestItemHash {
    std::size_t operator()(const TestItem& item) const { return std::hash<int>{}(item.key); }
    std::size_t operator()(int key) const { return std::hash<int>{}(key); }
};

struct TestItemEqual {
    bool operator()(const TestItem& a, const TestItem& b) const { return a.key == b.key; }
    bool operator()(const TestItem& a, int key) const { return a.key == key; }
    bool operator()(int key, const TestItem& a) const { return a.key == key; }
};

void MonotonicSetConcurrentInsert(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] {
        const std::size_t num_tasks = state.range(0);
        const std::size_t items_per_task = state.range(1) / num_tasks;

        for ([[maybe_unused]] auto _ : state) {
            concurrent::impl::MonotonicConcurrentSet<TestItem, TestItemHash, TestItemEqual> set;

            std::vector<engine::TaskWithResult<void>> tasks;
            tasks.reserve(num_tasks - 1);

            for (std::size_t thread_id = 1; thread_id < num_tasks; ++thread_id) {
                tasks.push_back(engine::AsyncNoSpan([&, thread_id]() {
                    for (std::size_t i = 0; i < items_per_task; ++i) {
                        int key = static_cast<int>((thread_id * items_per_task) + i);
                        set.TryEmplace(key, key, fmt::format("value_{}", key));
                    }
                }));
            }

            if (num_tasks > 1) {
                std::this_thread::yield();
            }

            for (std::size_t i = 0; i < items_per_task; ++i) {
                int key = static_cast<int>(i);
                set.TryEmplace(key, key, fmt::format("value_{}", key));
            }

            for (auto& task : tasks) {
                task.Get();
            }
        }

        state.SetItemsProcessed(state.iterations() * num_tasks * items_per_task);
    });
}

BENCHMARK(MonotonicSetConcurrentInsert)
    ->Args({1, 10})
    ->Args({1, 100})
    ->Args({1, 1000})
    ->Args({1, 10000})
    ->Args({4, 100})
    ->Args({8, 100})
    ->Args({4, 1000})
    ->Args({8, 1000})
    ->Args({4, 10000})
    ->Args({8, 10000});

void RcuMapConcurrentInsert(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] {
        const std::size_t num_tasks = state.range(0);
        const std::size_t items_per_task = state.range(1) / num_tasks;

        for ([[maybe_unused]] auto _ : state) {
            rcu::RcuMap<int, TestItem> map;

            std::vector<engine::TaskWithResult<void>> tasks;
            tasks.reserve(num_tasks - 1);

            for (std::size_t task_id = 1; task_id < num_tasks; ++task_id) {
                tasks.push_back(engine::AsyncNoSpan([&, task_id] {
                    for (std::size_t i = 0; i < items_per_task; ++i) {
                        int key = static_cast<int>((task_id * items_per_task) + i);
                        map.Emplace(key, key, fmt::format("value_{}", key));
                    }
                }));
            }

            if (num_tasks > 1) {
                std::this_thread::yield();
            }

            for (std::size_t i = 0; i < items_per_task; ++i) {
                int key = static_cast<int>(i);
                map.Emplace(key, key, fmt::format("value_{}", key));
            }

            for (auto& task : tasks) {
                task.Get();
            }
        }

        state.SetItemsProcessed(state.iterations() * num_tasks * items_per_task);
    });
}

BENCHMARK(RcuMapConcurrentInsert)
    ->Args({1, 10})
    ->Args({1, 100})
    ->Args({1, 1000})
    ->Args({4, 100})
    ->Args({8, 100})
    ->Args({4, 1000})
    ->Args({8, 1000});

void MonotonicSetConcurrentLookup(benchmark::State& state) {
    const std::size_t dataset_size = state.range(1);

    engine::RunStandalone(state.range(0), [&] {
        concurrent::impl::MonotonicConcurrentSet<TestItem, TestItemHash, TestItemEqual> set;
        for (std::size_t i = 0; i < dataset_size; ++i) {
            set.TryEmplace(static_cast<int>(i), static_cast<int>(i), fmt::format("value_{}", i));
        }

        RunParallelBenchmark(state, [&](auto& range) {
            for ([[maybe_unused]] auto _ : range) {
                for (std::size_t i = 0; i < dataset_size; ++i) {
                    auto found = set.Find(static_cast<int>(i));
                    if (found) {
                        benchmark::DoNotOptimize(found->key);
                    }
                }
            }
        });
    });

    state.SetItemsProcessed(state.iterations() * dataset_size);
}

BENCHMARK(MonotonicSetConcurrentLookup)->RangeMultiplier(2)->Ranges({{1, 16}, {1000, 1000}});

void RcuMapConcurrentLookup(benchmark::State& state) {
    const std::size_t dataset_size = state.range(1);

    engine::RunStandalone(state.range(0), [&] {
        rcu::RcuMap<int, TestItem> map;
        for (std::size_t i = 0; i < dataset_size; ++i) {
            map.Emplace(static_cast<int>(i), static_cast<int>(i), fmt::format("value_{}", i));
        }

        RunParallelBenchmark(state, [&](auto& range) {
            for ([[maybe_unused]] auto _ : range) {
                for (std::size_t i = 0; i < dataset_size; ++i) {
                    auto found = map.Get(static_cast<int>(i));
                    if (found) {
                        benchmark::DoNotOptimize(found->key);
                    }
                }
            }
        });
    });

    state.SetItemsProcessed(state.iterations() * dataset_size);
}

BENCHMARK(RcuMapConcurrentLookup)->RangeMultiplier(2)->Ranges({{1, 16}, {1000, 1000}});

}  // namespace

USERVER_NAMESPACE_END
