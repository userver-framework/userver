#pragma once

#include <atomic>
#include <cstddef>
#include <future>

#include <userver/engine/async.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/utils/fixed_array.hpp>

USERVER_NAMESPACE_BEGIN

namespace impl {

class KeepRunningIterator final {
 public:
  struct Empty {};

  explicit KeepRunningIterator(std::atomic<bool>& keep_running)
      : keep_running_(keep_running) {}

  KeepRunningIterator& operator++() { return *this; }
  Empty operator*() const { return {}; }
  bool operator!=(const KeepRunningIterator&) const { return keep_running_; }

 private:
  std::atomic<bool>& keep_running_;
};

struct KeepRunningRange final {
  auto begin() const { return KeepRunningIterator{keep_running}; }
  auto end() const { return KeepRunningIterator{keep_running}; }

  std::atomic<bool>& keep_running;
};

}  // namespace impl

template <typename Func>
void RunParallelBenchmark(benchmark::State& state, Func func) {
  std::atomic<bool> keep_running{true};

  if (engine::current_task::IsTaskProcessorThread()) {
    utils::FixedArray<engine::TaskWithResult<void>> competing_threads =
        utils::GenerateFixedArray(state.range(0) - 1, [&](std::size_t) {
          return engine::CriticalAsyncNoSpan([func, &keep_running] {
            const impl::KeepRunningRange range{keep_running};
            func(range);
          });
        });

    func(state);

    keep_running = false;
    for (auto& thread : competing_threads) {
      thread.Get();
    }
  } else {
    utils::FixedArray<std::future<void>> competing_threads =
        utils::GenerateFixedArray(state.range(0) - 1, [&](std::size_t) {
          return std::async([func, &keep_running] {
            const impl::KeepRunningRange range{keep_running};
            func(range);
          });
        });

    func(state);

    keep_running = false;
    for (auto& thread : competing_threads) {
      thread.get();
    }
  }
}

USERVER_NAMESPACE_END
