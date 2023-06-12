#include <userver/utest/utest.hpp>

#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>

USERVER_NAMESPACE_BEGIN

// This test fails consistently under current (at this commit
// b70ef2679b5ed68b915912167635db6eab3451c7) scheduling policy.
UTEST_MT(EngineYield, DISABLED_IsBroken, 2) {
  // so we have 2 threads here

  // alright, let's make the second thread just yield
  auto yield_task = engine::AsyncNoSpan([] {
    while (!engine::current_task::ShouldCancel()) {
      std::this_thread::sleep_for(std::chrono::milliseconds{20});
      engine::Yield();
    }
  });
  // We try to ensure that yield_task is picked up by the second thread
  std::this_thread::sleep_for(std::chrono::milliseconds{200});

  // let's schedule 2 tasks:
  // 1. one really slow blocking task (imagine it's doing some CPU-heavy stuff,
  // massive [de]allocation for example)
  // 2. one reasonably fast task
  auto slow_task = engine::AsyncNoSpan(
      [] { std::this_thread::sleep_for(std::chrono::milliseconds{500}); });
  const auto fast_task_duration =
      engine::AsyncNoSpan([start = std::chrono::steady_clock::now()] {
        std::this_thread::sleep_for(std::chrono::milliseconds{2});
        const auto finish = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(finish -
                                                                     start);
      }).Get();

  // One would expect that fast task will be executed within some
  // moderate amount of time, because no matter which thread gets blocked
  // by the slow_task, there is another one which
  // is never blocked for more than 20ms
  EXPECT_LT(fast_task_duration.count(), std::chrono::milliseconds{200}.count());
  // This ^ assert fails miserably

  yield_task.SyncCancel();
  slow_task.Get();
}

USERVER_NAMESPACE_END
