#include <userver/concurrent/background_task_storage.hpp>

#include <vector>

#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

void background_task_storage(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::BackgroundTaskStorage bts;

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        bts.AsyncDetach("task", [] {});
        engine::Yield();
      }
    });
  });
}
BENCHMARK(background_task_storage)
    ->Arg(2)
    ->Arg(4)
    ->Arg(6)
    ->Arg(8)
    ->Arg(12)
    ->Arg(16)
    ->Arg(32);

USERVER_NAMESPACE_END
