#include <engine/async.hpp>
#include <engine/condition_variable.hpp>
#include <engine/sleep.hpp>
#include <utest/utest.hpp>

#include <atomic>

TEST(ConditionVariable, AlreadySatisfied) {
  RunInCoro([] {
    engine::Mutex mutex;
    engine::ConditionVariable cv;

    std::unique_lock<engine::Mutex> lock(mutex);
    EXPECT_TRUE(cv.Wait(lock, [] { return true; }));
  });
}

TEST(ConditionVariable, Satisfy1) {
  RunInCoro([] {
    engine::Mutex mutex;
    engine::ConditionVariable cv;
    bool ok = false;

    auto task = engine::Async([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      EXPECT_TRUE(cv.Wait(lock, [&ok] { return ok; }));
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
              EXPECT_TRUE(cv.Wait(lock, [&ok] { return ok; }));
            }
          }));

        engine::SleepFor(std::chrono::milliseconds(50));

        ok = true;
        cv.NotifyAll();

        for (auto& task : tasks) task.Get();
      },
      10);
}

TEST(ConditionVariable, Status) {
  static constexpr std::chrono::milliseconds kWaitPeriod{10};

  class SpinEvent {
   public:
    SpinEvent() : is_pending_(false) {}

    void Send() { is_pending_ = true; }

    void WaitForEvent() {
      while (!is_pending_.exchange(false)) engine::Yield();
    }

   private:
    std::atomic<bool> is_pending_;
  };

  RunInCoro([] {
    engine::Mutex mutex;
    engine::ConditionVariable cv;
    SpinEvent has_started_event;
    {
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.Wait(lock);
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        cv.NotifyOne();
      }
      EXPECT_EQ(engine::CvStatus::kNoTimeout, task.Get());
    }
    {
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.Wait(lock);
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        task.RequestCancel();
      }
      EXPECT_EQ(engine::CvStatus::kCancelled, task.Get());
    }

    {
      bool flag = false;
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.Wait(lock, [&] { return flag; });
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        flag = true;
        cv.NotifyOne();
      }
      EXPECT_TRUE(task.Get());
    }
    {
      bool flag = false;
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.Wait(lock, [&] { return flag; });
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        task.RequestCancel();
      }
      EXPECT_FALSE(task.Get());
    }

    {
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.WaitFor(lock, kWaitPeriod);
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        cv.NotifyOne();
      }
      EXPECT_EQ(engine::CvStatus::kNoTimeout, task.Get());
    }
    {
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.WaitFor(lock, kWaitPeriod);
      });
      has_started_event.WaitForEvent();
      EXPECT_EQ(engine::CvStatus::kTimeout, task.Get());
    }
    {
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.WaitFor(lock, kWaitPeriod);
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        task.RequestCancel();
      }
      EXPECT_EQ(engine::CvStatus::kCancelled, task.Get());
    }

    {
      bool flag = false;
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.WaitFor(lock, kWaitPeriod, [&] { return flag; });
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);

        flag = true;
        cv.NotifyOne();
      }
      EXPECT_TRUE(task.Get());
    }
    {
      bool flag = false;
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.WaitFor(lock, kWaitPeriod, [&] { return flag; });
      });
      has_started_event.WaitForEvent();
      EXPECT_FALSE(task.Get());
    }
    {
      bool flag = false;
      auto task = engine::Async([&] {
        std::unique_lock<engine::Mutex> lock(mutex);
        has_started_event.Send();
        return cv.WaitFor(lock, kWaitPeriod, [&] { return flag; });
      });
      has_started_event.WaitForEvent();
      {
        std::unique_lock<engine::Mutex> lock(mutex);
        task.RequestCancel();
      }
      EXPECT_FALSE(task.Get());
    }
  });
}
