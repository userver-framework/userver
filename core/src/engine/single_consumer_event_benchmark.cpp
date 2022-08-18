#include <userver/engine/single_consumer_event.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
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
    std::vector<engine::TaskWithResult<std::uint64_t>> producers;
    producers.reserve(producer_count);

    for (std::size_t i = 0; i < producer_count; ++i) {
      producers.push_back(engine::AsyncNoSpan([&] {
        std::uint64_t events_sent = 0;

        while (keep_running) {
          event.value.Send();
          ++events_sent;
          engine::Yield();
        }

        return events_sent;
      }));
    }

    for (auto _ : state) {
      benchmark::DoNotOptimize(waiter(event.value));
    }

    keep_running = false;
    std::uint64_t total_events_sent = 0;
    for (auto& task : producers) {
      total_events_sent += task.Get();
    }

    state.counters["events-per-wait"] = benchmark::Counter(
        total_events_sent, benchmark::Counter::kAvgIterations);
  });
}

BENCHMARK_CAPTURE(SingleConsumerEvent, Endless, kEndless)->Apply(&ThreadsArg);
BENCHMARK_CAPTURE(SingleConsumerEvent, Successful, kSuccessful)
    ->Apply(&ThreadsArg);
BENCHMARK_CAPTURE(SingleConsumerEvent, Contended, kContended)
    ->Apply(&ThreadsArg);
BENCHMARK_CAPTURE(SingleConsumerEvent, Failed, kFailed)->Apply(&ThreadsArg);

void SingleConsumerEventPingPong(benchmark::State& state) {
  engine::RunStandalone(2, [&] {
    OverAligned<engine::SingleConsumerEvent> ping;
    OverAligned<engine::SingleConsumerEvent> pong;

    auto companion = engine::AsyncNoSpan([&] {
      while (true) {
        ping.value.Send();
        if (!pong.value.WaitForEvent()) return;
      }
    });

    for (auto _ : state) {
      if (!ping.value.WaitForEvent()) return;
      pong.value.Send();
    }

    companion.SyncCancel();
  });
}

BENCHMARK(SingleConsumerEventPingPong);

USERVER_NAMESPACE_END
