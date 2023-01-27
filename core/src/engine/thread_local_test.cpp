#include <userver/utest/utest.hpp>

#include <pthread.h>

#include <atomic>
#include <thread>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

thread_local std::atomic<int> kThreadLocal{1};

// NOTE: adding USERVER_PREVENT_TLS_CACHING helps make the test pass, but users
// have no access to it, and not all thread_locals are protected this way.
/* USERVER_PREVENT_TLS_CACHING */ int LoadThreadLocal() noexcept {
  return kThreadLocal.load(std::memory_order_relaxed);
}

/* USERVER_PREVENT_TLS_CACHING */ void MultiplyThreadLocal(
    int new_value) noexcept {
  kThreadLocal.store(kThreadLocal.load(std::memory_order_relaxed) * new_value,
                     std::memory_order_relaxed);
}

}  // namespace

// This test consistently fails on Clang-14 Release LTO. Please check at least
// on this specific compiler before re-enabling it.
UTEST_MT(ThreadLocal, DISABLED_TaskUsesCorrectInstanceAfterSleep, 2) {
  // Let's force a task to migrate between TaskProcessor threads.
  //
  // There are 2 TaskProcessor threads:
  // - let's call thread1 the thread on which the parent task initially runs
  // - let's call thread2 the other thread
  //
  // Here is the timeline of what thread executes what, second-by-second:
  // thread1: 2 3 5 5 5
  // thread2: 1 1 1 4 6

  const auto thread1_id = pthread_self();

  auto sleep2 = engine::AsyncNoSpan([&] {
    // (1)
    EXPECT_NE(pthread_self(), thread1_id);
    std::this_thread::sleep_for(3s);
  });

  // (2)
  EXPECT_EQ(pthread_self(), thread1_id);
  std::this_thread::sleep_for(1s);

  auto mutator_task = engine::AsyncNoSpan([&] {
    // (3)
    EXPECT_EQ(pthread_self(), thread1_id);
    std::this_thread::sleep_for(1s);

    MultiplyThreadLocal(2);

    engine::SleepFor(1s);

    // (4)
    EXPECT_NE(pthread_self(), thread1_id);
    std::this_thread::sleep_for(1s);

    MultiplyThreadLocal(3);
    return LoadThreadLocal();
  });

  auto sleep1 = engine::AsyncNoSpan([&] {
    // (5)
    EXPECT_EQ(pthread_self(), thread1_id);
    std::this_thread::sleep_for(3s);

    return LoadThreadLocal();
  });

  engine::SleepFor(3s);

  // (6)
  EXPECT_NE(pthread_self(), thread1_id);
  std::this_thread::sleep_for(1s);

  EXPECT_EQ(LoadThreadLocal(), 3);
  EXPECT_EQ(sleep1.Get(), 2);
  EXPECT_EQ(mutator_task.Get(), 3);

  UEXPECT_NO_THROW(sleep2.Get());
}

USERVER_NAMESPACE_END
