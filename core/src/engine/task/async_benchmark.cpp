#include <benchmark/benchmark.h>

#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/run_in_coro.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

// Note: We intentionally do not run this benchmark from RunInCoro to avoid
// any side-effects (RunInCoro spawns additional std::threads and uses some
// synchronization primitives).
void async_comparisons_std_thread(benchmark::State& state) {
  std::size_t constructed_joined_count = 0;
  for (auto _ : state) {
    std::thread([] {}).join();
    ++constructed_joined_count;
  }

  state.SetItemsProcessed(constructed_joined_count);
}
BENCHMARK(async_comparisons_std_thread);

void async_comparisons_coro(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::size_t constructed_joined_count = 0;
        for (auto _ : state) {
          engine::AsyncNoSpan([] {}).Wait();
          ++constructed_joined_count;
        }

        state.SetItemsProcessed(constructed_joined_count);
      },
      state.range(0));
}
BENCHMARK(async_comparisons_coro)->RangeMultiplier(2)->Range(1, 32);

void wrap_call_single(benchmark::State& state) {
  RunInCoro([&]() {
    for (auto _ : state) {
      utils::impl::WrapCall(utils::impl::InplaceConstructSpan{""}, []() {});
    }
  });
}
BENCHMARK(wrap_call_single);

void wrap_call_multiple(benchmark::State& state) {
  RunInCoro([&]() {
    const auto empty_lambda = []() {};
    using WrapPtr = decltype(utils::impl::WrapCall(
        utils::impl::InplaceConstructSpan{""}, empty_lambda));
    constexpr std::size_t kInMemoryInstancesCount = 100;
    for (auto _ : state) {
      std::array<WrapPtr, kInMemoryInstancesCount> a;
      for (std::size_t i = 0; i < kInMemoryInstancesCount; i++) {
        a[i] = utils::impl::WrapCall(utils::impl::InplaceConstructSpan{""},
                                     empty_lambda);
      }
    }
  });
}
BENCHMARK(wrap_call_multiple);

void wrap_call_and_perform(benchmark::State& state) {
  RunInCoro([&]() {
    for (auto _ : state) {
      auto wrapped_call_ptr =
          utils::impl::WrapCall(utils::impl::InplaceConstructSpan{""}, []() {});
      wrapped_call_ptr->Perform();
    }
  });
}
BENCHMARK(wrap_call_and_perform);

void async_comparisons_coro_spanned(benchmark::State& state) {
  RunInCoro(
      [&]() {
        std::size_t constructed_joined_count = 0;
        for (auto _ : state) {
          utils::Async("", [] {}).Wait();
          ++constructed_joined_count;
        }

        state.SetItemsProcessed(constructed_joined_count);
      },
      state.range(0));
}
BENCHMARK(async_comparisons_coro_spanned)->RangeMultiplier(2)->Range(1, 32);

USERVER_NAMESPACE_END
