#include <benchmark/benchmark.h>

#include <array>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_local_storage.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

using WrappedSpanCall =
    utils::impl::WrappedCallImplType<decltype(utils::impl::SpanLazyPrvalue("")),
                                     void (*)()>;

}

// Note: We intentionally do not run this benchmark from RunStandalone to avoid
// any side-effects (RunStandalone spawns additional std::threads and uses some
// synchronization primitives).
void async_comparisons_std_thread(benchmark::State& state) {
  std::uint64_t constructed_joined_count = 0;
  for (auto _ : state) {
    std::thread([] {}).join();
    ++constructed_joined_count;
  }
  benchmark::DoNotOptimize(constructed_joined_count);
}
BENCHMARK(async_comparisons_std_thread);

void async_comparisons_coro(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::uint64_t constructed_joined_count = 0;
    for (auto _ : state) {
      engine::AsyncNoSpan([] {}).Wait();
      ++constructed_joined_count;
    }
    benchmark::DoNotOptimize(constructed_joined_count);
  });
}
BENCHMARK(async_comparisons_coro)->RangeMultiplier(2)->Range(1, 32);

void wrap_call_single(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      WrappedSpanCall(utils::impl::SpanLazyPrvalue(""), []() {});
    }
  });
}
BENCHMARK(wrap_call_single);

void wrap_call_multiple(benchmark::State& state) {
  engine::RunStandalone([&] {
    constexpr std::size_t kInMemoryInstancesCount = 100;
    utils::FixedArray<std::optional<WrappedSpanCall>> calls{
        kInMemoryInstancesCount};

    for (auto _ : state) {
      for (auto& call : calls) {
        call.emplace(utils::impl::SpanLazyPrvalue(""), []() {});
      }
      for (auto& call : calls) {
        call.reset();
      }
    }
  });
}
BENCHMARK(wrap_call_multiple);

void wrap_call_and_perform(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      WrappedSpanCall wrapped_call{utils::impl::SpanLazyPrvalue(""), []() {}};
      {
        // Perform requires that task-local storage is empty, then fills it
        engine::impl::task_local::Storage discarded_storage;
        discarded_storage.InitializeFrom(
            std::move(engine::impl::task_local::GetCurrentStorage()));
      }
      wrapped_call.Perform();
    }
  });
}
BENCHMARK(wrap_call_and_perform);

void async_comparisons_coro_spanned(benchmark::State& state) {
  engine::RunStandalone(state.range(0), [&] {
    std::uint64_t constructed_joined_count = 0;
    for (auto _ : state) {
      utils::Async("", [] {}).Wait();
      ++constructed_joined_count;
    }
    benchmark::DoNotOptimize(constructed_joined_count);
  });
}
BENCHMARK(async_comparisons_coro_spanned)->RangeMultiplier(2)->Range(1, 32);

USERVER_NAMESPACE_END
