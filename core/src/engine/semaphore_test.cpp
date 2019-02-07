#include <engine/async.hpp>
#include <engine/semaphore.hpp>
#include <engine/sleep.hpp>
#include <engine/task/task_context.hpp>

#include <utest/utest.hpp>

TEST(Semaphore, Ctr) { engine::Semaphore s{100}; }

TEST(Semaphore, OnePass) {
  RunInCoro([] {
    engine::Semaphore s{1};
    auto task =
        engine::Async([&s]() { std::lock_guard<engine::Semaphore> guard{s}; });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Semaphore, TwoPass) {
  RunInCoro([] {
    engine::Semaphore s{2};
    std::lock_guard<engine::Semaphore> guard1{s};
    auto task =
        engine::Async([&s]() { std::lock_guard<engine::Semaphore> guard2{s}; });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Semaphore, LockAndCancel) {
  RunInCoro([] {
    engine::Semaphore s{1};
    std::unique_lock<engine::Semaphore> guard{s};
    auto task =
        engine::Async([&s]() { std::lock_guard<engine::Semaphore> guard{s}; });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());
    guard.unlock();
  });
}

TEST(Semaphore, Lock2AndCancel) {
  RunInCoro([] {
    engine::Semaphore s{2};
    std::lock_guard<engine::Semaphore> guard{s};
    std::unique_lock<engine::Semaphore> guard1{s};
    auto task =
        engine::Async([&s]() { std::lock_guard<engine::Semaphore> guard{s}; });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());
    guard1.unlock();
  });
}

TEST(Semaphore, LocksUnlocks) {
  RunInCoro([] {
    engine::Semaphore s{1};
    auto multilocker = [&s]() {
      for (unsigned i = 0; i < 100; i++) {
        std::lock_guard<engine::Semaphore> guard{s};
        engine::Yield();
      }
    };

    auto task = engine::Async(multilocker);
    multilocker();

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(Semaphore, LocksUnlocksMT) {
  RunInCoro(
      []() {
        engine::Semaphore s{1};
        auto multilocker = [&s]() {
          for (unsigned i = 0; i < 100; i++) {
            std::lock_guard<engine::Semaphore> guard{s};
            engine::Yield();
          }
        };

        auto task = engine::Async(multilocker);
        multilocker();

        task.WaitFor(std::chrono::milliseconds(50));
        EXPECT_TRUE(task.IsFinished());
      },
      2);
}

TEST(Semaphore, LocksUnlocksMtTourture) {
  RunInCoro(
      []() {
        engine::Semaphore s{2};
        auto multilocker = [&s]() {
          for (unsigned i = 0; i < 100; i++) {
            std::lock_guard<engine::Semaphore> guard{s};
            engine::Yield();
          }
        };

        engine::TaskWithResult<void> tasks[] = {
            engine::Async(multilocker), engine::Async(multilocker),
            engine::Async(multilocker), engine::Async(multilocker),
            engine::Async(multilocker), engine::Async(multilocker),
            engine::Async(multilocker), engine::Async(multilocker)};

        for (auto& t : tasks) {
          t.WaitFor(std::chrono::milliseconds(50));
          EXPECT_TRUE(t.IsFinished());
        }
      },
      4);
}
