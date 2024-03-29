#include <benchmark/benchmark.h>

#include <atomic>

#include <userver/concurrent/striped_counter.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using StripedCounter = concurrent::StripedCounter;

class NaiveCounter final {
 public:
  void Add(std::uint64_t value) { value_.fetch_add(value); }

  std::int64_t Read() const { return value_.load(); }

 private:
  std::atomic<std::int64_t> value_{0};
};

}  // namespace

template <typename Counter>
void CounterBenchmark(benchmark::State& state) {
  Counter counter;
  const auto before = counter.Read();

  RunParallelBenchmark(state, [&](auto& range) {
    for ([[maybe_unused]] auto _ : range) {
      // inner loop to reduce accounting overhead
      for (std::size_t i = 0; i < 10; ++i) {
        counter.Add(1);
      }
    }
  });

  const auto after = counter.Read();
  if (before == after) {
    state.SkipWithError("Shouldn't happen");
  }
}

BENCHMARK_TEMPLATE(CounterBenchmark, NaiveCounter)
    ->RangeMultiplier(2)
    ->Range(1, 64);
BENCHMARK_TEMPLATE(CounterBenchmark, StripedCounter)
    ->RangeMultiplier(2)
    ->Range(1, 64);

USERVER_NAMESPACE_END
