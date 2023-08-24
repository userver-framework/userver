#include <benchmark/benchmark.h>

#include <engine/ev/thread_control.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

void sleep_benchmark_us(benchmark::State& state) {
  engine::RunStandalone([&] {
    const std::chrono::microseconds sleep_duration{state.range(0)};
    for (auto _ : state) {
      const auto deadline = engine::Deadline::FromDuration(sleep_duration);
      engine::InterruptibleSleepUntil(deadline);
    }
  });
}
BENCHMARK(sleep_benchmark_us)
    ->RangeMultiplier(2)
    ->Range(1, 1024 * 128)
    ->Unit(benchmark::kMicrosecond);

void run_in_ev_loop_benchmark(benchmark::State& state) {
  engine::RunStandalone([&] {
    auto& ev_thread = engine::current_task::GetEventThread();
    for (auto _ : state) {
      ev_thread.RunInEvLoopAsync([]() {});
    }
  });
}
BENCHMARK(run_in_ev_loop_benchmark);

[[maybe_unused]] void successful_wait_for_benchmark(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      auto task = engine::AsyncNoSpan([] { engine::Yield(); });
      task.WaitFor(20ms);

      if (!task.IsFinished()) abort();
    }
  });
}
BENCHMARK(successful_wait_for_benchmark);

void unreached_task_deadline_benchmark(benchmark::State& state,
                                       bool has_task_deadline) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      const auto sleep_deadline = engine::Deadline::FromDuration(20s);
      const auto task_deadline_raw = engine::Deadline::FromDuration(40s);
      benchmark::DoNotOptimize(task_deadline_raw);
      const auto task_deadline =
          has_task_deadline ? task_deadline_raw : engine::Deadline{};

      auto task = engine::AsyncNoSpan(task_deadline, [&] {
        engine::InterruptibleSleepUntil(sleep_deadline);
      });
      engine::Yield();
      task.SyncCancel();
    }
  });
}
BENCHMARK_CAPTURE(unreached_task_deadline_benchmark, no_task_deadline, false);
BENCHMARK_CAPTURE(unreached_task_deadline_benchmark, unreached_task_deadline,
                  true);

USERVER_NAMESPACE_END
