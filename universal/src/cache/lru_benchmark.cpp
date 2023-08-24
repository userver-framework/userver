#include <benchmark/benchmark.h>

#include <userver/cache/lru_set.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Lru = cache::LruSet<unsigned>;

constexpr unsigned kElementsCount = 1000;

Lru FillLru(unsigned elements_count) {
  Lru lru(kElementsCount);
  for (unsigned i = 0; i < elements_count; ++i) {
    lru.Put(i);
  }

  return lru;
}

}  // namespace

void LruPut(benchmark::State& state) {
  for (auto _ : state) {
    auto lru = FillLru(kElementsCount);
    benchmark::DoNotOptimize(lru);
  }
}
BENCHMARK(LruPut);

void LruHas(benchmark::State& state) {
  auto lru = FillLru(kElementsCount);
  for (auto _ : state) {
    for (unsigned i = 0; i < kElementsCount; ++i) {
      benchmark::DoNotOptimize(lru.Has(i));
    }
  }
}
BENCHMARK(LruHas);

void LruPutOverflow(benchmark::State& state) {
  auto lru = FillLru(kElementsCount);
  unsigned i = kElementsCount;
  for (auto _ : state) {
    for (unsigned j = 0; j < kElementsCount; ++j) {
      lru.Put(++i);
    }
    benchmark::DoNotOptimize(lru);
  }
}
BENCHMARK(LruPutOverflow);

USERVER_NAMESPACE_END
