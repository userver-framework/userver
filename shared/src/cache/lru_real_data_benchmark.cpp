#if 0

#include <benchmark/benchmark.h>
#include <userver/cache/impl/slru.hpp>
#include <userver/cache/impl/window_tiny_lfu.hpp>
#include <userver/cache/lfu_map.hpp>
#include <userver/cache/lru_map.hpp>

#include <userver/fs/blocking/read.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using Lru = cache::LruMap<std::string, unsigned>;
using Lfu = LfuBase<std::string, unsigned>;
using Slru = cache::LruMap<std::string, unsigned, std::hash<std::string>, std::equal_to<std::string>, cache::CachePolicy::kSLRU>;
using TinyLfu = cache::LruMap<std::string, unsigned, std::hash<std::string>, std::equal_to<std::string>, cache::CachePolicy::kTinyLFU>;
using WTinyLfu = cache::LruMap<std::string, unsigned, std::hash<std::string>, std::equal_to<std::string>, cache::CachePolicy::kWTinyLFU>;

  std::vector<std::string> LoadData(std::string file) {
    auto rows = userver::fs::blocking::ReadFileContents(file);
    std::vector<std::string> res;

    std::istringstream is(rows);
    std::string key;
    while (is >> key) {
      res.push_back(key);
    }
    return res;
  }
} // namespace

template <typename CachePolicyContainer, const char* test_data_file>
void OnRealData(benchmark::State& state) {
  std::int32_t all = 0;
  std::int32_t hit = 0;
  auto data = LoadData(test_data_file);
  auto lru = CachePolicyContainer(state.range(0));
  for (auto _ : state) {
    for (auto& key : data) {
      all++;
      if (!lru.Get(key)) {
        lru.Put(key, 1);
      } else {
        hit++;
      }
    }
  }
  state.counters["hit_rate"] = static_cast<double>(hit) / all;
}

static const char phoenix[] = "test_data/phoenix";
static const char goblet[] = "test_data/goblet";
static const char zipfian_10k_2kk[] = "test_data/zipfian_10k_2kk";
BENCHMARK(OnRealData<Lru, phoenix>)->RangeMultiplier(2)->Range(2<<10, 2<<20);
BENCHMARK(OnRealData<Lfu, phoenix>)->RangeMultiplier(2)->Range(2<<10, 2<<20);
BENCHMARK(OnRealData<Slru, phoenix>)->RangeMultiplier(2)->Range(2<<10, 2<<20);
BENCHMARK(OnRealData<TinyLfu, phoenix>)->RangeMultiplier(2)->Range(2<<10, 2<<20);
BENCHMARK(OnRealData<WTinyLfu, phoenix>)->RangeMultiplier(2)->Range(2<<10, 2<<20);
BENCHMARK(OnRealData<Lru, goblet>)->RangeMultiplier(2)->Range(2<<6, 2<<17);
BENCHMARK(OnRealData<Lfu, goblet>)->RangeMultiplier(2)->Range(2<<6, 2<<17);
BENCHMARK(OnRealData<Slru, goblet>)->RangeMultiplier(2)->Range(2<<6, 2<<17);
BENCHMARK(OnRealData<TinyLfu, goblet>)->RangeMultiplier(2)->Range(2<<6, 2<<17);
BENCHMARK(OnRealData<WTinyLfu, goblet>)->RangeMultiplier(2)->Range(2<<6, 2<<17);
BENCHMARK(OnRealData<Lru, zipfian_10k_2kk>)->RangeMultiplier(2)->Range(2<<5, 2<<12);
BENCHMARK(OnRealData<Lfu, zipfian_10k_2kk>)->RangeMultiplier(2)->Range(2<<5, 2<<12);
BENCHMARK(OnRealData<Slru, zipfian_10k_2kk>)->RangeMultiplier(2)->Range(2<<5, 2<<12);
BENCHMARK(OnRealData<TinyLfu, zipfian_10k_2kk>)->RangeMultiplier(2)->Range(2<<5, 2<<12);
BENCHMARK(OnRealData<WTinyLfu, zipfian_10k_2kk>)->RangeMultiplier(2)->Range(2<<5, 2<<12);

USERVER_NAMESPACE_END

#endif
