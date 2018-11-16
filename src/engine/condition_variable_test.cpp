#include <engine/async.hpp>
#include <engine/condition_variable.hpp>
#include <engine/sleep.hpp>
#include <utest/utest.hpp>

TEST(ConditionVariable, AlreadySatisfied) {
  RunInCoro([] {
    engine::Mutex mutex;
    engine::ConditionVariable cv;

    std::unique_lock<engine::Mutex> lock(mutex);
    cv.Wait(lock, [] { return true; });
  });
}

TEST(ConditionVariable, Satisfy1) {
  RunInCoro([] {
    engine::Mutex mutex;
    engine::ConditionVariable cv;
    bool ok = false;

    auto task = engine::Async([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      cv.Wait(lock, [&ok] { return ok; });
    });

    engine::Yield();
    EXPECT_FALSE(task.IsFinished());

    ok = true;
    cv.NotifyAll();
    engine::Yield();
    EXPECT_TRUE(task.IsFinished());
  });
}

TEST(ConditionVariable, SatisfyMultiple) {
  RunInCoro(
      [] {
        engine::Mutex mutex;
        engine::ConditionVariable cv;
        bool ok = false;

        std::vector<engine::TaskWithResult<void>> tasks;
        for (int i = 0; i < 40; i++)
          tasks.push_back(engine::Async([&] {
            for (int j = 0; j < 10; j++) {
              std::unique_lock<engine::Mutex> lock(mutex);
              cv.Wait(lock, [&ok] { return ok; });
            }
          }));

        engine::SleepFor(std::chrono::milliseconds(50));

        ok = true;
        cv.NotifyAll();

        for (auto& task : tasks) task.Get();
      },
      10);
}
