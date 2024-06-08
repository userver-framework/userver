#include <userver/concurrent/impl/striped_read_indicator.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>

#include <benchmark/benchmark.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/fixed_array.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class SimpleRefcount {
 public:
  class [[nodiscard]] ReadLock {
   public:
    explicit ReadLock(SimpleRefcount& refcount) : refcount_(refcount) {
      refcount_.counter_.fetch_add(1, std::memory_order_release);
    }

    ReadLock(ReadLock&&) = delete;

    ~ReadLock() { refcount_.counter_.fetch_sub(1, std::memory_order_release); }

   private:
    SimpleRefcount& refcount_;
  };

  ReadLock Lock() noexcept { return ReadLock{*this}; }

  bool IsFree() const noexcept {
    return counter_.load(std::memory_order_acquire);
  }

 private:
  std::atomic<std::size_t> counter_{0};
};

}  // namespace

template <typename ReadIndicator>
void ReadIndicatorLockUnlock(benchmark::State& state) {
  engine::RunStandalone(state.range(0) + 1, [&] {
    ReadIndicator indicator;

    auto checker_task = engine::CriticalAsyncNoSpan([&] {
      while (!engine::current_task::ShouldCancel()) {
        benchmark::DoNotOptimize(indicator.IsFree());
        engine::InterruptibleSleepFor(std::chrono::milliseconds{10});
      }
    });

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        auto lock = indicator.Lock();
        benchmark::DoNotOptimize(lock);
      }
    });

    checker_task.RequestCancel();
    checker_task.Get();
  });
}

BENCHMARK_TEMPLATE(ReadIndicatorLockUnlock, SimpleRefcount)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(ReadIndicatorLockUnlock,
                   concurrent::impl::StripedReadIndicator)
    ->RangeMultiplier(2)
    ->Range(1, 32);

template <typename ReadIndicator>
void ReadIndicatorIsFree(benchmark::State& state) {
  engine::RunStandalone(state.range(0) + 1, [&] {
    ReadIndicator indicator;
    std::atomic<bool> keep_running{true};

    auto tasks = utils::GenerateFixedArray(state.range(0), [&](std::size_t) {
      return engine::CriticalAsyncNoSpan([&] {
        while (keep_running) {
          const auto lock = indicator.Lock();
          benchmark::DoNotOptimize(keep_running.load());
        }
      });
    });

    for (auto _ : state) {
      benchmark::DoNotOptimize(indicator.IsFree());
    }

    keep_running = false;
    for (auto& task : tasks) {
      task.Get();
    }
  });
}

BENCHMARK_TEMPLATE(ReadIndicatorIsFree, SimpleRefcount)
    ->RangeMultiplier(2)
    ->Range(1, 32);
BENCHMARK_TEMPLATE(ReadIndicatorIsFree, concurrent::impl::StripedReadIndicator)
    ->RangeMultiplier(2)
    ->Range(1, 32);

USERVER_NAMESPACE_END
