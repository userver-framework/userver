#include <benchmark/benchmark.h>

#include <array>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_local_storage.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

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
      utils::impl::WrapCall(utils::impl::SpanLazyPrvalue(""), []() {});
    }
  });
}
BENCHMARK(wrap_call_single);

void wrap_call_multiple(benchmark::State& state) {
  engine::RunStandalone([&] {
    const auto empty_lambda = []() {};
    using WrapPtr = decltype(
        utils::impl::WrapCall(utils::impl::SpanLazyPrvalue(""), empty_lambda));
    constexpr std::size_t kInMemoryInstancesCount = 100;
    for (auto _ : state) {
      std::array<WrapPtr, kInMemoryInstancesCount> a;
      for (std::size_t i = 0; i < kInMemoryInstancesCount; i++) {
        a[i] = utils::impl::WrapCall(utils::impl::SpanLazyPrvalue(""),
                                     empty_lambda);
      }
    }
  });
}
BENCHMARK(wrap_call_multiple);

void wrap_call_and_perform(benchmark::State& state) {
  engine::RunStandalone([&] {
    for (auto _ : state) {
      auto wrapped_call_ptr =
          utils::impl::WrapCall(utils::impl::SpanLazyPrvalue(""), []() {});
      {
        // Perform requires that task-local storage is empty, then fills it
        engine::impl::task_local::Storage discarded_storage;
        discarded_storage.InitializeFrom(
            std::move(engine::impl::task_local::GetCurrentStorage()));
      }
      wrapped_call_ptr->Perform();
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
