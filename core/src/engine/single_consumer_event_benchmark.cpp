#include <userver/engine/single_consumer_event.hpp>

#include <atomic>
#include <chrono>
#include <vector>

#include <benchmark/benchmark.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::size_t kHardwareDestructiveInterferenceSize = 64;

// Protects other data from the destructive interference of data inside T.
template <typename T>
struct alignas(kHardwareDestructiveInterferenceSize) OverAligned final {
  T value;
};

using Waiter = bool (*)(engine::SingleConsumerEvent&);

constexpr Waiter kEndless = [](engine::SingleConsumerEvent& event) {
  return event.WaitForEvent();
};

constexpr Waiter kSuccessful = [](engine::SingleConsumerEvent& event) {
  return event.WaitForEventFor(20s);
};

constexpr Waiter kContended = [](engine::SingleConsumerEvent& event) {
  return event.WaitForEventFor(1us);
};

constexpr Waiter kFailed = [](engine::SingleConsumerEvent& event) {
  return event.WaitForEventFor(0s);
};

template <typename BenchmarkType>
void ThreadsArg(BenchmarkType* benchmark) {
  benchmark->Arg(2)->Arg(3)->Arg(4)->Arg(6)->Arg(8)->Arg(12)->Arg(16)->Arg(32);
}

}  // namespace

void SingleConsumerEvent(benchmark::State& state, Waiter waiter) {
  engine::RunStandalone(state.range(0), [&] {
    const auto producer_count = static_cast<std::size_t>(state.range(0)) - 1;

    OverAligned<engine::SingleConsumerEvent> event;
    std::atomic<bool> keep_running{true};
    std::vector<engine::TaskWithResult<void>> producers;
    producers.reserve(producer_count);

    for (std::size_t i = 0; i < producer_count; ++i) {
      producers.push_back(engine::AsyncNoSpan([&] {
        while (keep_running) {
          event.value.Send();
          engine::Yield();
        }
      }));
    }

    for (auto _ : state) {
      benchmark::DoNotOptimize(waiter(event.value));
    }

    keep_running = false;
    for (auto& task : producers) task.Get();
  });
}

BENCHMARK_CAPTURE(SingleConsumerEvent, Endless, kEndless)->Apply(&ThreadsArg);
BENCHMARK_CAPTURE(SingleConsumerEvent, Successful, kSuccessful)
    ->Apply(&ThreadsArg);
BENCHMARK_CAPTURE(SingleConsumerEvent, Contended, kContended)
    ->Apply(&ThreadsArg);
BENCHMARK_CAPTURE(SingleConsumerEvent, Failed, kFailed)->Apply(&ThreadsArg);

USERVER_NAMESPACE_END
