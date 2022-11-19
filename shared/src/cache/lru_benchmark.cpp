#include <benchmark/benchmark.h>

#include <userver/cache/lru_map.hpp>
#include <userver/cache/lfu_map.hpp>
#include <userver/cache/impl/slru.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Lru = cache::LruMap<unsigned, unsigned>;
using Lfu = LfuBase<unsigned, unsigned>;
using Slru = cache::LruMap<unsigned, unsigned, std::hash<unsigned>, std::equal_to<unsigned>, cache::CachePolicy::kSLRU>;
using TinyLfu = cache::LruMap<unsigned, unsigned, std::hash<unsigned>, std::equal_to<unsigned>, cache::CachePolicy::kTinyLFU>;

constexpr unsigned kElementsCount = 5000;

template <typename CachePolicyContainer>
CachePolicyContainer FillLru(unsigned elements_count) {
  CachePolicyContainer lru(kElementsCount);
  for (unsigned i = 0; i < elements_count; ++i) {
    lru.Put(i, i);
  }

  return lru;
}

}  // namespace

template <typename CachePolicyContainer>
void Put(benchmark::State& state) {
  for (auto _ : state) {
    auto lru = FillLru<CachePolicyContainer>(kElementsCount);
    benchmark::DoNotOptimize(lru);
  }
}
BENCHMARK(Put<Lru>);
BENCHMARK(Put<Lfu>);
BENCHMARK(Put<Slru>);
BENCHMARK(Put<TinyLfu>);

template <typename CachePolicyContainer>
void Has(benchmark::State& state) {
  auto lru = FillLru<CachePolicyContainer>(kElementsCount);
  for (auto _ : state) {
    for (unsigned i = 0; i < kElementsCount; ++i) {
      benchmark::DoNotOptimize(lru.Get(i));
    }
  }
}
BENCHMARK(Has<Lru>);
BENCHMARK(Has<Lfu>);
BENCHMARK(Has<Slru>);
BENCHMARK(Has<TinyLfu>);

template <typename CachePolicyContainer>
void PutOverflow(benchmark::State& state) {
  auto lru = FillLru<CachePolicyContainer>(kElementsCount);
  unsigned i = kElementsCount;
  for (auto _ : state) {
    for (unsigned j = 0; j < kElementsCount; ++j) {
      ++i;
      lru.Put(i, i);
    }
    benchmark::DoNotOptimize(lru);
  }
}
BENCHMARK(PutOverflow<Lru>);
BENCHMARK(PutOverflow<Lfu>);
BENCHMARK(PutOverflow<Slru>);
BENCHMARK(PutOverflow<TinyLfu>);

USERVER_NAMESPACE_END
