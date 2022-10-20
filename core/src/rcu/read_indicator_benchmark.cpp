#include <benchmark/benchmark.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <vector>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/rcu/read_indicator.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class SimpleReadIndicator {
 public:
  struct Unlocker {
    void operator()(std::atomic<std::size_t>* counter) const {
      counter->fetch_sub(1, std::memory_order_release);
    }
  };

  struct USERVER_NODISCARD ReadLock {
    std::unique_ptr<std::atomic<std::size_t>, Unlocker> counter;

    explicit ReadLock(SimpleReadIndicator& indicator)
        : counter(&indicator.counter_) {}
  };

  ReadLock Lock() noexcept {
    counter_.fetch_add(1, std::memory_order_release);
    return ReadLock{*this};
  }

  bool IsFree() noexcept { return counter_.load(std::memory_order_acquire); }

 private:
  std::atomic<std::size_t> counter_{0};
};

}  // namespace

template <typename ReadIndicator>
void read_indicator_lock_unlock(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    ReadIndicator indicator;
    std::vector<engine::TaskWithResult<void>> tasks;
    std::atomic<bool> keep_running = true;

    const auto lock_unlock = [&] { (void)indicator.Lock(); };

    for (int i = 0; i < state.range(0) - 1; ++i) {
      tasks.push_back(utils::Async("rcu-worker", [&] {
        while (keep_running) {
          lock_unlock();
        }
      }));
    }

    const bool measure_writer = state.range(1);

    if (measure_writer) {
      tasks.push_back(utils::Async("rcu-worker", [&] {
        while (keep_running) {
          lock_unlock();
        }
      }));

      for (auto _ : state) {
        benchmark::DoNotOptimize(indicator.IsFree());
      }
    } else {
      tasks.push_back(utils::Async("rcu-slow-checker", [&] {
        while (keep_running) {
          benchmark::DoNotOptimize(indicator.IsFree());
          engine::SleepFor(std::chrono::milliseconds{10});
        }
      }));

      for (auto _ : state) {
        lock_unlock();
      }
    }

    keep_running = false;
    for (auto& task : tasks) {
      task.SyncCancel();
    }
  });
}
BENCHMARK_TEMPLATE(read_indicator_lock_unlock, SimpleReadIndicator)
    ->RangeMultiplier(2)
    ->Ranges({{1, 32}, {false, true}});
BENCHMARK_TEMPLATE(read_indicator_lock_unlock, rcu::ReadIndicator)
    ->RangeMultiplier(2)
    ->Ranges({{1, 32}, {false, true}});

USERVER_NAMESPACE_END
