#include <userver/engine/single_consumer_event.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <vector>

#include <benchmark/benchmark.h>

#include <userver/concurrent/impl/interference_shield.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

using Waiter = bool (*)(engine::SingleConsumerEvent&);

constexpr Waiter kEndless = [](engine::SingleConsumerEvent& event) { return event.WaitForEvent(); };

constexpr Waiter kSuccessful = [](engine::SingleConsumerEvent& event) { return event.WaitForEventFor(20s); };

constexpr Waiter kContended = [](engine::SingleConsumerEvent& event) { return event.WaitForEventFor(1us); };

constexpr Waiter kFailed = [](engine::SingleConsumerEvent& event) { return event.WaitForEventFor(0s); };

}  // namespace

static void SingleConsumerEvent(benchmark::State& state, Waiter waiter) {
    engine::RunStandalone(state.range(0), [&] {
        // At least one producer, so the single-threaded variant (1 worker thread) still makes progress.
        const auto producer_count = std::max(static_cast<std::size_t>(state.range(0)) - 1, std::size_t{1});

        concurrent::impl::InterferenceShield<engine::SingleConsumerEvent> event;
        std::atomic<bool> keep_running{true};
        std::vector<engine::TaskWithResult<std::uint64_t>> producers;
        producers.reserve(producer_count);

        for (std::size_t i = 0; i < producer_count; ++i) {
            producers.push_back(engine::AsyncNoTracing([&] {
                std::uint64_t events_sent = 0;

                while (keep_running) {
                    event->Send();
                    ++events_sent;
                    engine::Yield();
                }

                return events_sent;
            }));
        }

        for ([[maybe_unused]] auto _ : state) {
            benchmark::DoNotOptimize(waiter(*event));
        }

        keep_running = false;
        std::uint64_t total_events_sent = 0;
        for (auto& task : producers) {
            total_events_sent += task.Get();
        }

        state.counters["events-sent"] = benchmark::Counter(total_events_sent, benchmark::Counter::kIsRate);
    });
}

#define BENCHMARK_THREAD_ARGS ->Arg(1)->Arg(2)->Arg(3)->Arg(4)->Arg(6)->Arg(8)->Arg(12)->Arg(16)->Arg(32)

BENCHMARK_CAPTURE(SingleConsumerEvent, Endless, kEndless) BENCHMARK_THREAD_ARGS;
BENCHMARK_CAPTURE(SingleConsumerEvent, Successful, kSuccessful) BENCHMARK_THREAD_ARGS;
BENCHMARK_CAPTURE(SingleConsumerEvent, Contended, kContended) BENCHMARK_THREAD_ARGS;
BENCHMARK_CAPTURE(SingleConsumerEvent, Failed, kFailed) BENCHMARK_THREAD_ARGS;

#undef BENCHMARK_THREAD_ARGS

static void SingleConsumerEventPingPong(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] {
        concurrent::impl::InterferenceShield<engine::SingleConsumerEvent> ping;
        concurrent::impl::InterferenceShield<engine::SingleConsumerEvent> pong;

        auto companion = engine::AsyncNoTracing([&] {
            while (true) {
                ping->Send();
                if (!pong->WaitForEvent()) {
                    return;
                }
            }
        });

        for ([[maybe_unused]] auto _ : state) {
            if (!ping->WaitForEvent()) {
                return;
            }
            pong->Send();
        }

        companion.SyncCancel();
    });
}

BENCHMARK(SingleConsumerEventPingPong)->Arg(1)->Arg(2);

USERVER_NAMESPACE_END
