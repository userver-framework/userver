#include <userver/utest/utest.hpp>

#include <pthread.h>

#include <atomic>
#include <thread>

#include <userver/compiler/impl/tls.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

thread_local std::atomic<int> kThreadLocal{1};

// NOTE: adding compiler::ThreadLocal on these functions helps make the test
// pass, but not all thread_locals are protected this way.
int LoadThreadLocal() noexcept {
  return kThreadLocal.load(std::memory_order_relaxed);
}

void MultiplyThreadLocal(int new_value) noexcept {
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

namespace {

auto& SafeGetThreadLocal() {
  return compiler::impl::ThreadLocal([] { return std::atomic<int>{1}; });
}

int SafeLoadThreadLocal() noexcept {
  return SafeGetThreadLocal().load(std::memory_order_relaxed);
}

void SafeMultiplyThreadLocal(int new_value) noexcept {
  auto& atomic = SafeGetThreadLocal();
  atomic.store(atomic.load(std::memory_order_relaxed) * new_value,
               std::memory_order_relaxed);
}

}  // namespace

// This is a copy-paste from TaskUsesCorrectInstanceAfterSleep test.
// While the test above consistently fails as of now, this test should pass.
UTEST_MT(ThreadLocal, SafeThreadLocalWorks, 2) {
  const auto thread1_id = pthread_self();

  auto sleep2 = engine::AsyncNoSpan([&] {
    // (1)
    EXPECT_NE(pthread_self(), thread1_id);
    std::this_thread::sleep_for(300ms);
  });

  // (2)
  EXPECT_EQ(pthread_self(), thread1_id);
  std::this_thread::sleep_for(100ms);

  auto mutator_task = engine::AsyncNoSpan([&] {
    // (3)
    EXPECT_EQ(pthread_self(), thread1_id);
    std::this_thread::sleep_for(100ms);

    SafeMultiplyThreadLocal(2);

    engine::SleepFor(100ms);

    // (4)
    EXPECT_NE(pthread_self(), thread1_id);
    std::this_thread::sleep_for(100ms);

    SafeMultiplyThreadLocal(3);
    return SafeLoadThreadLocal();
  });

  auto sleep1 = engine::AsyncNoSpan([&] {
    // (5)
    EXPECT_EQ(pthread_self(), thread1_id);
    std::this_thread::sleep_for(300ms);

    return SafeLoadThreadLocal();
  });

  engine::SleepFor(300ms);

  // (6)
  EXPECT_NE(pthread_self(), thread1_id);
  std::this_thread::sleep_for(100ms);

  EXPECT_EQ(SafeLoadThreadLocal(), 3);
  EXPECT_EQ(sleep1.Get(), 2);
  EXPECT_EQ(mutator_task.Get(), 3);

  UEXPECT_NO_THROW(sleep2.Get());
}

namespace {

thread_local std::size_t manually_protected_var{};

// Test that userver-based services can use thread-local variables.
struct UserverCompilerThreadLocal {
  static std::size_t* GetLocal() {
    // Don't use it in production, use `compiler::ThreadLocal` instead
    return &compiler::impl::ThreadLocal([] { return std::size_t{0}; });
  }
};

// Test that third-party libraries can use thread-local variables
// in a userver-compatible way.
struct ManuallyProtectedThreadLocal {
  __attribute__((noinline)) static std::size_t* GetLocal() {
    // Don't use it in production
    auto ptr = &manually_protected_var;
    // clang-format off
    // NOLINTNEXTLINE(hicpp-no-assembler)
    asm volatile("" : "+rm" (ptr));
    // clang-format on
    return ptr;
  }
};

template <typename T>
class ThreadLocalTyped : public ::testing::Test {};

using ThreadLocalTypes =
    ::testing::Types<UserverCompilerThreadLocal, ManuallyProtectedThreadLocal>;

}  // namespace

TYPED_UTEST_SUITE(ThreadLocalTyped, ThreadLocalTypes);

// Created 4 threads and 8 tasks for concurrency mode
// Use thread_local variable with types `compiler::ThreadLocal` and
// `thread_local` with macros.
// Compare pointer thread_local variable before
// `engine::Yield` and after. Pointers must be different if thread_ids are
// different
TYPED_UTEST_MT(ThreadLocalTyped, SmallFunctionUseInnerTL, 4) {
  constexpr std::size_t kNumTasks = 8;

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(kNumTasks);
  for (std::size_t i = 0; i < kNumTasks; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      for (auto i = 0; i < 1000; ++i) {
        const auto thread_local_ptr_before = TypeParam::GetLocal();
        const auto thread_id_before = std::this_thread::get_id();
        engine::Yield();
        const auto thread_local_ptr_after = TypeParam::GetLocal();
        const auto thread_id_after = std::this_thread::get_id();
        if (thread_id_before != thread_id_after) {
          EXPECT_NE(thread_local_ptr_before, thread_local_ptr_after);
        }
      }
    }));
  }
  for (auto& task : tasks) {
    UEXPECT_NO_THROW(task.Get());
  }
}

USERVER_NAMESPACE_END
