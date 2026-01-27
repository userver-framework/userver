#include <benchmark/benchmark.h>

#include <engine/ev/thread_control.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

void SleepBenchmarkUs(benchmark::State& state) {
    engine::RunStandalone([&] {
        const std::chrono::microseconds sleep_duration{state.range(0)};
        for ([[maybe_unused]] auto _ : state) {
            const auto deadline = engine::Deadline::FromDuration(sleep_duration);
            engine::InterruptibleSleepUntil(deadline);
        }
    });
}
BENCHMARK(SleepBenchmarkUs)->RangeMultiplier(2)->Range(1, 1024 * 128)->Unit(benchmark::kMicrosecond);

void RunInEvLoopBenchmark(benchmark::State& state) {
    engine::RunStandalone([&] {
        auto& ev_thread = engine::current_task::GetEventThread();
        for ([[maybe_unused]] auto _ : state) {
            ev_thread.RunInEvLoopAsync([]() {});
        }
    });
}
BENCHMARK(RunInEvLoopBenchmark);

[[maybe_unused]] void SuccessfulWaitForBenchmark(benchmark::State& state) {
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            auto task = engine::AsyncNoSpan([] { engine::Yield(); });
            task.WaitFor(20ms);

            if (!task.IsFinished()) {
                abort();
            }
        }
    });
}
BENCHMARK(SuccessfulWaitForBenchmark);

void UnreachedTaskDeadlineBenchmark(benchmark::State& state, bool has_task_deadline) {
    engine::RunStandalone([&] {
        for ([[maybe_unused]] auto _ : state) {
            const auto sleep_deadline = engine::Deadline::FromDuration(20s);
            const auto task_deadline_raw = engine::Deadline::FromDuration(40s);
            benchmark::DoNotOptimize(task_deadline_raw);
            const auto task_deadline = has_task_deadline ? task_deadline_raw : engine::Deadline{};

            auto task = engine::AsyncNoSpan(task_deadline, [&] { engine::InterruptibleSleepUntil(sleep_deadline); });
            engine::Yield();
            task.SyncCancel();
        }
    });
}
BENCHMARK_CAPTURE(UnreachedTaskDeadlineBenchmark, no_task_deadline, false);
BENCHMARK_CAPTURE(UnreachedTaskDeadlineBenchmark, unreached_task_deadline, true);

USERVER_NAMESPACE_END
