#include <benchmark/benchmark.h>

#include <engine/task/task_context.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

void sleep_benchmark_mcs(benchmark::State& state) {
  engine::RunStandalone([&] {
    const std::chrono::microseconds sleep_duration{state.range(0)};
    for (auto _ : state) {
      const auto deadline = engine::Deadline::FromDuration(sleep_duration);
      engine::InterruptibleSleepUntil(deadline);
    }
  });
}
BENCHMARK(sleep_benchmark_mcs)
    ->RangeMultiplier(2)
    ->Range(1, 1024 * 1024)
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

void successful_wait_for_benchmark(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      auto task = engine::AsyncNoSpan([] { engine::Yield(); });
      task.WaitFor(std::chrono::milliseconds{20});

      if (!task.IsFinished()) abort();
    }
  });
}
BENCHMARK(successful_wait_for_benchmark);

USERVER_NAMESPACE_END
