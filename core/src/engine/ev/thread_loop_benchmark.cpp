#include <benchmark/benchmark.h>

#include <engine/ev/thread_pool.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void EvLoopRoundTrip(benchmark::State& state) {
    engine::ev::ThreadPool pool{{1, "ev-benchmark"}};
    auto& control = pool.NextThread();
    for ([[maybe_unused]] auto _ : state) {
        control.RunInEvLoopBlocking([] {});
    }
}
BENCHMARK(EvLoopRoundTrip)->UseRealTime();

}  // namespace

USERVER_NAMESPACE_END
