#include <benchmark/benchmark.h>

#include <atomic>

#include <concurrent/impl/striped_counter.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using StripedCounter = concurrent::impl::StripedCounter;
StripedCounter striped_counter{};

class NaiveCounter final {
 public:
  void Add(std::uint64_t value) { value_.fetch_add(value); }

  std::int64_t Read() const { return value_.load(); }

 private:
  std::atomic<std::int64_t> value_{0};
};
NaiveCounter naive_counter{};

template <typename T>
auto& GetCounter() {
  if constexpr (std::is_same_v<T, StripedCounter>) {
    return striped_counter;
  } else if constexpr (std::is_same_v<T, NaiveCounter>) {
    return naive_counter;
  } else {
    static_assert(!sizeof(T));
  }
}

}  // namespace

template <typename Counter>
void CounterBenchmark(benchmark::State& state) {
  auto& counter = GetCounter<Counter>();
  const auto before = counter.Read();

  for ([[maybe_unused]] auto _ : state) {
    // inner loop to reduce accounting overhead
    for (std::size_t i = 0; i < 10; ++i) {
      counter.Add(1);
    }
  }

  const auto after = counter.Read();
  if (before == after) {
    state.SkipWithError("Shouldn't happen");
  }
}

BENCHMARK_TEMPLATE(CounterBenchmark, NaiveCounter)->ThreadRange(1, 64);
BENCHMARK_TEMPLATE(CounterBenchmark, StripedCounter)->ThreadRange(1, 64);

USERVER_NAMESPACE_END
