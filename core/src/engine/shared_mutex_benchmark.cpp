#include <benchmark/benchmark.h>

#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/run_in_coro.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

void shared_mutex_benchmark(benchmark::State& state) {
  const auto run = [&](auto func) { RunInCoro(func, state.range(0)); };

  run([&]() {
    int variable = 0;
    engine::SharedMutex mutex;
    std::atomic<bool> is_running(true);

    std::vector<engine::TaskWithResult<void>> tasks;
    for (int i = 0; i < state.range(0) - 1; ++i) {
      tasks.push_back(engine::impl::Async([&] {
        while (is_running) {
          std::shared_lock lock(mutex);
          benchmark::DoNotOptimize(variable);
        }
      }));
    }

    {
      // ensure the locks are actually needed
      std::unique_lock lock(mutex);
      variable = 1;
    }

    for (auto _ : state) {
      std::shared_lock lock(mutex);
      benchmark::DoNotOptimize(variable);
    }

    is_running = false;

    for (auto& task : tasks) {
      task.Get();
    }
  });
}
BENCHMARK(shared_mutex_benchmark)->DenseRange(1, 6);

USERVER_NAMESPACE_END
