#include <userver/concurrent/background_task_storage.hpp>

#include <vector>

#include <benchmark/benchmark.h>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

void background_task_storage(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::BackgroundTaskStorage bts;

    std::vector<engine::TaskWithResult<void>> tasks;
    for (std::int64_t i = 0; i < state.range(0) - 1; ++i) {
      tasks.push_back(engine::AsyncNoSpan([&] {
        while (!engine::current_task::ShouldCancel()) {
          bts.AsyncDetach("task", [] {});
          engine::Yield();
        }
      }));
    }

    for (auto _ : state) {
      bts.AsyncDetach("task", [] {});
      engine::Yield();
    }
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
