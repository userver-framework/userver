#include <benchmark/benchmark.h>
#include <random>
#include <vector>

#include <userver/cache/expirable_lru_cache.hpp>
#include <userver/engine/run_standalone.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void ExpirableLruCacheGetMiss(benchmark::State& state) {
    const auto key_count = static_cast<std::size_t>(state.range(0));
    engine::RunStandalone([&] {
        cache::ExpirableLruCache<int, int> cache(4, key_count);
        cache.SetMaxLifetime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(1)));

        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<int> key_dist(0, static_cast<int>(key_count) - 1);

        for ([[maybe_unused]] auto _ : state) {
            const int key = key_dist(generator);
            benchmark::DoNotOptimize(
                cache.Get(key, [](int k) { return k; }, cache::ExpirableLruCache<int, int>::ReadMode::kUseCache)
            );
        }
    });
}

void ExpirableLruCacheInvalidateByKeyIf(benchmark::State& state) {
    const auto key_count = static_cast<std::size_t>(state.range(0));
    engine::RunStandalone([&] {
        cache::ExpirableLruCache<int, int> cache(4, key_count);
        cache.SetMaxLifetime(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(1)));
        for (std::size_t i = 0; i < key_count; ++i) {
            cache.Put(static_cast<int>(i), static_cast<int>(i));
        }

        std::random_device device;
        std::mt19937 generator(device());
        std::uniform_int_distribution<int> key_dist(0, static_cast<int>(key_count) - 1);

        for ([[maybe_unused]] auto _ : state) {
            int key = key_dist(generator);
            cache.InvalidateByKeyIf(key, [](const int&) { return true; });
            benchmark::DoNotOptimize(key);
        }
    });
}

}  // namespace

const std::vector<std::vector<int64_t>> kExpirableBenchmarkArgsProduct{
    // {16 * 1000 * 1000, 32 * 1000 * 1000, 64 * 1000 * 1000, 128 * 1000 * 1000, 256 * 1000 * 1000},
    {16 * 1000},
};

BENCHMARK(ExpirableLruCacheGetMiss)->ArgsProduct(kExpirableBenchmarkArgsProduct)->Unit(benchmark::kNanosecond);

BENCHMARK(ExpirableLruCacheInvalidateByKeyIf)
    ->ArgsProduct(kExpirableBenchmarkArgsProduct)
    ->Unit(benchmark::kNanosecond);

USERVER_NAMESPACE_END
