#include <userver/engine/semaphore.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/utest.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fast_scope_guard.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

UTEST(Semaphore, Ctr) { engine::Semaphore s{100}; }

UTEST(Semaphore, OnePass) {
  engine::Semaphore s{1};
  auto task = engine::AsyncNoSpan([&s]() { std::shared_lock guard{s}; });

  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}

/// [UTEST macro example 1]
UTEST(Semaphore, PassAcrossCoroutines) {
  engine::Semaphore s{1};
  std::shared_lock guard{s};

  auto task = utils::Async("test", [guard = std::move(guard)] {});
  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}
/// [UTEST macro example 1]

UTEST(Semaphore, PassAcrossCoroutinesLocal) {
  engine::Semaphore s{1};
  std::shared_lock guard{s};
  auto task2 = engine::AsyncNoSpan([guard = std::move(guard)]() mutable {
    std::shared_lock local_guard = std::move(guard);
  });
  task2.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task2.IsFinished());
}

UTEST(Semaphore, TwoPass) {
  engine::Semaphore s{2};
  std::shared_lock guard1{s};
  auto task = engine::AsyncNoSpan([&s]() { std::shared_lock guard2{s}; });

  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}

UTEST(Semaphore, LockAndCancel) {
  engine::Semaphore s{1};
  std::shared_lock guard{s};
  auto task = engine::AsyncNoSpan([&s]() { std::shared_lock guard{s}; });

  task.WaitFor(std::chrono::milliseconds(100));
  task.RequestCancel();
  task.WaitFor(std::chrono::milliseconds(100));
  EXPECT_FALSE(task.IsFinished());

  guard.unlock();
  UEXPECT_NO_THROW(task.Get());
}

UTEST(CancellableSemaphore, LockAndCancel) {
  engine::CancellableSemaphore s{1};
  std::shared_lock guard{s};
  auto task = engine::AsyncNoSpan([&s]() { std::shared_lock guard{s}; });

  task.WaitFor(std::chrono::milliseconds(100));
  task.RequestCancel();

  task.WaitFor(std::chrono::milliseconds(utest::kMaxTestWaitTime));
  EXPECT_TRUE(task.IsFinished());
  UEXPECT_THROW(task.Get(), engine::SemaphoreLockCancelledError);
}

UTEST(CancellableSemaphore, TryLockAndCancel) {
  engine::CancellableSemaphore s{1};
  std::shared_lock guard{s};
  auto task = engine::AsyncNoSpan([&s]() {
    [[maybe_unused]] auto tmp = s.try_lock_shared_for(utest::kMaxTestWaitTime);
  });

  task.WaitFor(std::chrono::milliseconds(100));
  task.RequestCancel();
  task.WaitFor(utest::kMaxTestWaitTime / 2);
  EXPECT_TRUE(task.IsFinished());
  guard.unlock();
}

UTEST(Semaphore, Lock2AndCancel) {
  engine::Semaphore s{2};
  std::shared_lock guard{s};
  std::shared_lock<engine::Semaphore> guard1{s};
  auto task = engine::AsyncNoSpan([&s]() { std::shared_lock guard{s}; });

  task.WaitFor(std::chrono::milliseconds(50));
  EXPECT_FALSE(task.IsFinished());
  guard1.unlock();
}

UTEST(Semaphore, LocksUnlocks) {
  engine::Semaphore s{1};
  auto multilocker = [&s]() {
    for (unsigned i = 0; i < 100; i++) {
      std::shared_lock guard{s};
      engine::Yield();
    }
  };

  auto task = engine::AsyncNoSpan(multilocker);
  multilocker();

  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}

UTEST_MT(Semaphore, LocksUnlocksMT, 2) {
  engine::Semaphore s{1};
  auto multilocker = [&s]() {
    for (unsigned i = 0; i < 100; i++) {
      std::shared_lock guard{s};
      engine::Yield();
    }
  };

  auto task = engine::AsyncNoSpan(multilocker);
  multilocker();

  task.WaitFor(utest::kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
}

UTEST_MT(Semaphore, LocksUnlocksMtTorture, 4) {
  engine::Semaphore s{2};
  auto multilocker = [&s]() {
    for (unsigned i = 0; i < 100; i++) {
      std::shared_lock guard{s};
      engine::Yield();
    }
  };

  constexpr std::size_t kTasksCount = 8;
  engine::TaskWithResult<void> tasks[kTasksCount] = {
      engine::AsyncNoSpan(multilocker), engine::AsyncNoSpan(multilocker),
      engine::AsyncNoSpan(multilocker), engine::AsyncNoSpan(multilocker),
      engine::AsyncNoSpan(multilocker), engine::AsyncNoSpan(multilocker),
      engine::AsyncNoSpan(multilocker), engine::AsyncNoSpan(multilocker)};

  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
  for (auto& t : tasks) {
    t.WaitUntil(deadline);
    EXPECT_TRUE(t.IsFinished());
  }
}

UTEST(Semaphore, TryLock) {
  engine::Semaphore sem(2);

  std::shared_lock lock(sem);
  EXPECT_TRUE(engine::AsyncNoSpan([&sem] {
                return !!std::shared_lock(sem, std::try_to_lock);
              }).Get());
  EXPECT_TRUE(engine::AsyncNoSpan([&sem] {
                return !!std::shared_lock(sem, std::chrono::milliseconds(10));
              }).Get());
  EXPECT_TRUE(engine::AsyncNoSpan([&sem] {
                return !!std::shared_lock(sem,
                                          std::chrono::system_clock::now());
              }).Get());

  auto long_holder = engine::AsyncNoSpan([&sem] {
    std::shared_lock lock(sem);
    engine::InterruptibleSleepUntil(engine::Deadline{});
  });
  engine::Yield();

  EXPECT_FALSE(engine::AsyncNoSpan([&sem] {
                 return !!std::shared_lock(sem, std::try_to_lock);
               }).Get());

  EXPECT_FALSE(engine::AsyncNoSpan([&sem] {
                 return !!std::shared_lock(sem, std::chrono::milliseconds(10));
               }).Get());
  EXPECT_FALSE(engine::AsyncNoSpan([&sem] {
                 return !!std::shared_lock(sem,
                                           std::chrono::system_clock::now());
               }).Get());

  auto long_waiter = engine::AsyncNoSpan(
      [&sem] { return !!std::shared_lock(sem, utest::kMaxTestWaitTime); });
  engine::Yield();
  EXPECT_FALSE(long_waiter.IsFinished());
  long_holder.RequestCancel();
  EXPECT_TRUE(long_waiter.Get());
}

UTEST_MT(Semaphore, LockPassing, 4) {
  static constexpr auto kTestDuration = std::chrono::milliseconds{500};

  const auto test_deadline = engine::Deadline::FromDuration(kTestDuration);
  engine::Semaphore sem{1};

  const auto work = [&sem] {
    std::shared_lock lock(sem, std::defer_lock);
    ASSERT_TRUE(lock.try_lock_for(utest::kMaxTestWaitTime));
  };

  while (!test_deadline.IsReached()) {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < GetThreadCount(); ++i) {
      tasks.push_back(engine::AsyncNoSpan(work));
    }
    for (auto& task : tasks) task.Get();
  }
}

UTEST_MT(Semaphore, LockFastPathRace, 5) {
  const auto test_deadline = engine::Deadline::FromDuration(100ms);
  engine::Semaphore sem{-1UL};
  std::vector<engine::TaskWithResult<void>> tasks;

  for (std::size_t i = 0; i < GetThreadCount(); ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      std::size_t locks_taken = 0;
      utils::FastScopeGuard unlock(
          [&]() noexcept { sem.unlock_shared_count(locks_taken); });

      while (!test_deadline.IsReached()) {
        ASSERT_TRUE(sem.try_lock_shared());
        ++locks_taken;
      }
    }));
  }

  for (auto& task : tasks) task.Get();
}

UTEST(Semaphore, AllWaitersWakeUpWhenNeeded) {
  constexpr std::size_t kLocksCount = 3;

  engine::Semaphore sem{kLocksCount};

  // Acquire locks 1-by-1. We'll then release them all at once.
  for (std::size_t i = 0; i < kLocksCount; ++i) sem.lock_shared();

  std::vector<engine::TaskWithResult<void>> tasks;

  std::atomic<std::size_t> locks_acquired{0};
  engine::SingleConsumerEvent all_locks_acquired;

  for (std::size_t i = 0; i < kLocksCount; ++i) {
    tasks.push_back(engine::AsyncNoSpan([&] {
      std::shared_lock lock(sem);
      if (++locks_acquired == kLocksCount) all_locks_acquired.Send();

      // only release 'lock' on test shutdown
      engine::InterruptibleSleepUntil(engine::Deadline{});
    }));
  }

  // Wait for all threads to start waiting. This is not required by Semaphore,
  // but is required to reproduce the bug.
  engine::SleepFor(10ms);

  // After this, all waiters should wake up and acquire the lock. However, if
  // Semaphore is bugged, some will not wake up.
  sem.unlock_shared_count(kLocksCount);

  // After all waiters have acquired the lock, 'all_locks_acquired' is sent.
  EXPECT_TRUE(all_locks_acquired.WaitForEventFor(utest::kMaxTestWaitTime));
}

UTEST_MT(Semaphore, NotifyAndDeadlineRace, 2) {
  constexpr int kTestIterationsCount = 1000;
  constexpr auto kSmallWaitTime = 5us;

  for (int i = 0; i < kTestIterationsCount; ++i) {
    engine::Semaphore sem{1};
    std::shared_lock lock(sem);

    engine::SingleConsumerEvent lock_acquired;

    auto deadline_task = engine::AsyncNoSpan([&] {
      if (sem.try_lock_shared_for(kSmallWaitTime)) {
        sem.unlock_shared();
        lock_acquired.Send();
      }
    });

    auto no_deadline_task = engine::AsyncNoSpan([&] {
      if (sem.try_lock_shared_until(engine::Deadline{})) {
        sem.unlock_shared();
        lock_acquired.Send();
      }
    });

    engine::SleepFor(kSmallWaitTime);

    // After this, if 'deadline_task' has not timed out yet, it should acquire
    // the lock. If 'deadline_task' has timed out, 'no_deadline_task' should
    // acquire the lock.
    //
    // A bug could happen if we wake up 'deadline_task' while it's cancelling
    // itself due to deadline. 'deadline_task' will wake up, but not lock the
    // semaphore.
    lock.unlock();

    ASSERT_TRUE(lock_acquired.WaitForEventFor(utest::kMaxTestWaitTime));

    deadline_task.Get();
    no_deadline_task.Get();
  }
}

UTEST(Semaphore, OverAcquireDisallowed) {
  engine::Semaphore semaphore{2};
  ASSERT_FALSE(semaphore.try_lock_shared_count(3));
  UASSERT_THROW(semaphore.lock_shared_count(3),
                engine::UnreachableSemaphoreLockError);

  EXPECT_EQ(semaphore.GetCapacity(), 2);
  EXPECT_EQ(semaphore.RemainingApprox(), 2);
}

UTEST(Semaphore, SetCapacity) {
  engine::Semaphore semaphore{2};
  ASSERT_EQ(semaphore.GetCapacity(), 2);
  ASSERT_EQ(semaphore.RemainingApprox(), 2);
  ASSERT_FALSE(semaphore.try_lock_shared_count(3));

  semaphore.SetCapacity(10);
  ASSERT_EQ(semaphore.GetCapacity(), 10);
  ASSERT_EQ(semaphore.RemainingApprox(), 10);
  ASSERT_TRUE(semaphore.try_lock_shared_count(2));
  ASSERT_EQ(semaphore.RemainingApprox(), 8);
  ASSERT_TRUE(semaphore.try_lock_shared_count(5));
  ASSERT_EQ(semaphore.RemainingApprox(), 3);
  semaphore.unlock_shared_count(4);
  ASSERT_EQ(semaphore.RemainingApprox(), 7);

  semaphore.SetCapacity(5);
  ASSERT_EQ(semaphore.GetCapacity(), 5);
  ASSERT_EQ(semaphore.RemainingApprox(), 2);
  ASSERT_FALSE(semaphore.try_lock_shared_count(3));

  semaphore.unlock_shared_count(3);
}

UTEST(Semaphore, SetCapacityOverAcquire) {
  engine::Semaphore semaphore{10};
  ASSERT_TRUE(semaphore.try_lock_shared_count(8));

  semaphore.SetCapacity(5);
  // Actual remaining locks is -3, because 8 locks were acquired, then capacity
  // was lowered to 5.
  ASSERT_EQ(semaphore.RemainingApprox(), 0);
  ASSERT_FALSE(semaphore.try_lock_shared_count(1));

  semaphore.unlock_shared_count(2);
  // Actual remaining locks is -1.
  ASSERT_EQ(semaphore.RemainingApprox(), 0);
  ASSERT_FALSE(semaphore.try_lock_shared_count(1));

  semaphore.unlock_shared_count(3);
  // Yay, we are out of debt now!
  ASSERT_EQ(semaphore.RemainingApprox(), 2);
  ASSERT_TRUE(semaphore.try_lock_shared_count(1));
  ASSERT_EQ(semaphore.RemainingApprox(), 1);

  semaphore.unlock_shared_count(4);
}

UTEST(Semaphore, SetCapacityNotifyUnreachable) {
  engine::Semaphore semaphore{5};
  EXPECT_TRUE(semaphore.try_lock_shared_count(3));

  auto task5 = engine::AsyncNoSpan([&] {
    const bool success = semaphore.try_lock_shared_until_count(
        engine::Deadline::FromDuration(utest::kMaxTestWaitTime), 5);
    EXPECT_FALSE(success);
  });

  engine::SleepFor(10ms);
  semaphore.SetCapacity(3);

  UEXPECT_NO_THROW(task5.Get());
  semaphore.unlock_shared_count(3);
}

/// [UTEST macro example 2]
UTEST_MT(SemaphoreLock, LockMoveCopyOwning, 2) {
  engine::Semaphore sem{1};
  engine::SemaphoreLock lock(sem);
  ASSERT_TRUE(lock.OwnsLock());

  engine::SemaphoreLock move_here{std::move(lock)};
  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(lock.OwnsLock());
  EXPECT_TRUE(move_here.OwnsLock());
}
/// [UTEST macro example 2]

UTEST_MT(SemaphoreLock, LockMoveCopyEmpty, 2) {
  engine::SemaphoreLock empty_lock;
  ASSERT_FALSE(empty_lock.OwnsLock());

  engine::SemaphoreLock move_here{std::move(empty_lock)};
  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(empty_lock.OwnsLock());
  EXPECT_FALSE(move_here.OwnsLock());
}

UTEST_MT(SemaphoreLock, LockMoveAssignOwning, 2) {
  engine::Semaphore sem{1};
  engine::SemaphoreLock lock(sem);
  ASSERT_TRUE(lock.OwnsLock());

  engine::SemaphoreLock move_here;
  move_here = std::move(lock);
  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(lock.OwnsLock());
  EXPECT_TRUE(move_here.OwnsLock());
}

UTEST_MT(SemaphoreLock, LockMoveAssignEmpty, 2) {
  engine::SemaphoreLock empty_lock;
  ASSERT_FALSE(empty_lock.OwnsLock());

  engine::SemaphoreLock move_here;
  move_here = std::move(empty_lock);
  // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
  EXPECT_FALSE(empty_lock.OwnsLock());
  EXPECT_FALSE(move_here.OwnsLock());
}

UTEST(SemaphoreLock, SampleSemaphore) {
  /// [Sample engine::Semaphore usage]
  constexpr auto kMaxSimultaneousLocks = 3;
  engine::Semaphore sema(kMaxSimultaneousLocks);
  // ...
  {
    std::shared_lock lock(sema);
    // There may be no more than 3 users
    // in the critical section at the same time
  }
  /// [Sample engine::Semaphore usage]
}

USERVER_NAMESPACE_END
