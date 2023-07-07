#include <benchmark/benchmark.h>

#include <userver/cache/impl/slru.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Slru = cache::impl::SlruBase<unsigned, unsigned>;

constexpr unsigned kElementsCount = 1000;
constexpr unsigned kProbationPart = 800;
constexpr unsigned kProtectedPart = 200;

Slru FillSlru(unsigned elements_count) {
  Slru slru(kProbationPart, kProtectedPart);
  for (unsigned i = 0; i < elements_count; ++i) {
    slru.Put(i, i);
  }

  return slru;
}

Slru FillSlruBlocks(unsigned elements_count) {
  Slru slru(kProbationPart, kProtectedPart);
  for (unsigned i = 0; i < elements_count / 2; ++i) {
    for (unsigned j = 0; j <= i % 3; ++j) {
      slru.Put(i, i);
    }
  }
  return slru;
}

}  // namespace

void SlruPut(benchmark::State& state) {
  for (auto _ : state) {
    auto slru = FillSlru(kElementsCount);
    benchmark::DoNotOptimize(slru);
  }
}
BENCHMARK(SlruPut);

void SlruBlocksPut(benchmark::State& state) {
  for (auto _ : state) {
    auto slru = FillSlruBlocks(kElementsCount);
    benchmark::DoNotOptimize(slru);
  }
}
BENCHMARK(SlruBlocksPut);

void SlruHas(benchmark::State& state) {
  auto slru = FillSlru(kElementsCount);
  for (auto _ : state) {
    for (unsigned i = 0; i < kElementsCount; ++i) {
      benchmark::DoNotOptimize(slru.Get(i));
    }
  }
}
BENCHMARK(SlruHas);

void SlruBlocksHas(benchmark::State& state) {
  auto slru = FillSlruBlocks(kElementsCount);
  for (auto _ : state) {
    for (unsigned i = 0; i < kElementsCount; ++i) {
      benchmark::DoNotOptimize(slru.Get(i));
    }
  }
}
BENCHMARK(SlruBlocksHas);

void SlruPutOverflow(benchmark::State& state) {
  auto slru = FillSlru(kElementsCount);
  unsigned i = kElementsCount;
  for (auto _ : state) {
    for (unsigned j = 0; j < kElementsCount; ++j) {
      slru.Put(++i, 0);
    }
    benchmark::DoNotOptimize(slru);
  }
}
BENCHMARK(SlruPutOverflow);

USERVER_NAMESPACE_END
