#include <userver/concurrent/background_task_storage.hpp>

#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_use_event.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

void background_task_storage(benchmark::State& state) {
    engine::RunStandalone(state.range(0), [&] {
        concurrent::BackgroundTaskStorageCore bts;

        RunParallelBenchmark(state, [&](auto& range) {
            for ([[maybe_unused]] auto _ : range) {
                engine::SingleUseEvent event;
                bts.Detach(engine::AsyncNoSpan([&] { event.Send(); }));
                event.WaitNonCancellable();
            }
        });
    });
}
BENCHMARK(background_task_storage)->Arg(2)->Arg(4)->Arg(6)->Arg(8)->Arg(12)->Arg(16)->Arg(32);

USERVER_NAMESPACE_END
