#include <concurrent/intrusive_walkable_pool.hpp>

#include <atomic>
#include <cstdint>

#include <benchmark/benchmark.h>

#include <userver/engine/run_standalone.hpp>
#include <utils/impl/parallelize_benchmark.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct IntNode final {
  std::atomic<std::uint64_t> value{0};
  concurrent::impl::IntrusiveWalkablePoolHook<IntNode> hook;
};

}  // namespace

void intrusive_walkable_pool(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    concurrent::impl::IntrusiveWalkablePool<
        IntNode, concurrent::impl::MemberHook<&IntNode::hook>>
        pool;

    RunParallelBenchmark(state, [&pool](auto& range) {
      for ([[maybe_unused]] auto _ : range) {
        auto& node = pool.Acquire();
        node.value.store(node.value.load() + 1, std::memory_order_relaxed);
        pool.Release(node);
      }
    });
  });
}
BENCHMARK(intrusive_walkable_pool)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(6)
    ->Arg(8)
    ->Arg(12)
    ->Arg(16)
    ->Arg(32);

USERVER_NAMESPACE_END
