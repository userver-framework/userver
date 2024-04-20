#include <benchmark/benchmark.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

void shared_mutex_benchmark(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    int variable = 0;
    engine::SharedMutex mutex;

    auto initial_lock_holder = engine::AsyncNoSpan([&] {
      // ensure the locks are actually needed
      std::unique_lock lock(mutex);
      variable = 1;
    });

    RunParallelBenchmark(state, [&](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        std::shared_lock lock(mutex);
        benchmark::DoNotOptimize(variable);
      }
    });
  });
}
BENCHMARK(shared_mutex_benchmark)->DenseRange(1, 6);

USERVER_NAMESPACE_END
