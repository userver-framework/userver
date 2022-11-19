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

constexpr unsigned kElementsCount = 75000;

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

namespace {
template <>
Slru FillLru<Slru>(unsigned elements_count) {
  Slru lru(kElementsCount);
  auto probation_size = static_cast<unsigned>(elements_count * 0.8);
  for (unsigned i = 0; i < probation_size; ++i) {
    lru.Put(i, i);
  }
  for (unsigned i = 0; i < elements_count - probation_size; ++i) {
    lru.Put(i, i);
    lru.Put(i + probation_size, i);
  }
  return lru;
}
} // namespace

template<>
void PutOverflow<Slru>(benchmark::State& state) {
  auto lru = FillLru<Slru>(kElementsCount);
  auto protected_size = static_cast<unsigned>(kElementsCount * 0.2);
  unsigned i = protected_size;
  for (auto _ : state) {
    for (unsigned j = 0; j < protected_size; ++j) {
      ++i;
      lru.Put(i, i);
    }
    benchmark::DoNotOptimize(lru);
  }
}

BENCHMARK(Put<Slru>);
BENCHMARK(PutOverflow<Slru>);

void TinyLfuDoorkeeper(benchmark::State& state) {
    for (auto _ : state) {
        auto lru = FillLru<TinyLfu>(kElementsCount);
        for (unsigned i = 0; i < kElementsCount; ++i) {
            lru.Put(i+kElementsCount, i);
        }
    }
}
BENCHMARK(TinyLfuDoorkeeper);

void TinyLfuBloom(benchmark::State& state) {
    for (auto _ : state) {
        auto lru = FillLru<TinyLfu>(kElementsCount);
        for (unsigned i = 0; i < kElementsCount; ++i) {
            lru.Get(i);
        }
        for (unsigned i = 0; i < kElementsCount; ++i) {
            lru.Put(i+kElementsCount, i);
        }
    }
}
BENCHMARK(TinyLfuBloom);

USERVER_NAMESPACE_END
