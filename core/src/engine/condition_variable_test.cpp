#include <userver/engine/condition_variable.hpp>

#include <atomic>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

class SpinEvent final {
 public:
  SpinEvent() : is_pending_(false) {}

  void Send() { is_pending_ = true; }

  void WaitForEvent() {
    while (!is_pending_.exchange(false)) engine::Yield();
  }

 private:
  std::atomic<bool> is_pending_;
};

constexpr std::chrono::milliseconds kWaitPeriod{10};

}  // namespace

UTEST(ConditionVariable, AlreadySatisfied) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;

  std::unique_lock<engine::Mutex> lock(mutex);
  EXPECT_TRUE(cv.Wait(lock, [] { return true; }));
}

UTEST(ConditionVariable, Satisfy1) {
  /// [Sample ConditionVariable usage]
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  bool ok = false;

  auto task = engine::AsyncNoSpan([&] {
    std::unique_lock<engine::Mutex> lock(mutex);
    EXPECT_TRUE(cv.Wait(lock, [&ok] { return ok; }));
  });

  engine::Yield();
  EXPECT_FALSE(task.IsFinished());

  ok = true;
  cv.NotifyAll();
  engine::Yield();
  EXPECT_TRUE(task.IsFinished());
  /// [Sample ConditionVariable usage]
}

UTEST_MT(ConditionVariable, SatisfyMultiple, 10) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  bool ok = false;

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(40);
  for (int i = 0; i < 40; i++)
    tasks.push_back(engine::AsyncNoSpan([&] {
      for (int j = 0; j < 10; j++) {
        std::unique_lock<engine::Mutex> lock(mutex);
        EXPECT_TRUE(cv.Wait(lock, [&ok] { return ok; }));
      }
    }));

  engine::SleepFor(std::chrono::milliseconds(50));

  {
    std::lock_guard<engine::Mutex> lock(mutex);
    ok = true;
    cv.NotifyAll();
  }

  for (auto& task : tasks) task.Get();
}

UTEST(ConditionVariable, WaitStatus) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    auto task = engine::AsyncNoSpan([&] {
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
    auto task = engine::AsyncNoSpan([&] {
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
}

UTEST(ConditionVariable, WaitPredicateStatus) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    bool flag = false;
    auto task = engine::AsyncNoSpan([&] {
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
    auto task = engine::AsyncNoSpan([&] {
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
}

UTEST(ConditionVariable, WaitForStatus) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    auto task = engine::AsyncNoSpan([&] {
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
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      return cv.WaitFor(lock, kWaitPeriod);
    });
    has_started_event.WaitForEvent();
    EXPECT_EQ(engine::CvStatus::kTimeout, task.Get());
  }
  {
    auto task = engine::AsyncNoSpan([&] {
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
}

UTEST(ConditionVariable, WaitForPredicateStatus) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    bool flag = false;
    auto task = engine::AsyncNoSpan([&] {
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
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      return cv.WaitFor(lock, kWaitPeriod, [&] { return flag; });
    });
    has_started_event.WaitForEvent();
    EXPECT_FALSE(task.Get());
  }
  {
    bool flag = false;
    auto task = engine::AsyncNoSpan([&] {
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
}

UTEST(ConditionVariable, BlockedCancelWait) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      engine::TaskCancellationBlocker block_cancel;
      return cv.Wait(lock);
    });
    has_started_event.WaitForEvent();
    {
      std::unique_lock<engine::Mutex> lock(mutex);
      task.RequestCancel();
    }
    task.WaitFor(kWaitPeriod);
    EXPECT_FALSE(task.IsFinished());
    {
      std::unique_lock<engine::Mutex> lock(mutex);
      cv.NotifyOne();
    }
    EXPECT_EQ(engine::CvStatus::kNoTimeout, task.Get());
  }
}

UTEST(ConditionVariable, BlockedCancelWaitPredicate) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    bool flag = false;
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      engine::TaskCancellationBlocker block_cancel;
      return cv.Wait(lock, [&] { return flag; });
    });
    has_started_event.WaitForEvent();
    {
      std::unique_lock<engine::Mutex> lock(mutex);
      task.RequestCancel();
      flag = true;
      cv.NotifyOne();
    }
    EXPECT_TRUE(task.Get());
  }
  {
    bool flag = false;
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      engine::TaskCancellationBlocker block_cancel;
      return cv.Wait(lock, [&] { return flag; });
    });
    has_started_event.WaitForEvent();
    {
      std::unique_lock<engine::Mutex> lock(mutex);
      task.RequestCancel();
    }
    task.WaitFor(kWaitPeriod);
    EXPECT_FALSE(task.IsFinished());

    {
      std::unique_lock<engine::Mutex> lock(mutex);
      flag = true;
      cv.NotifyOne();
    }
    EXPECT_TRUE(task.Get());
  }
}

UTEST(ConditionVariable, BlockedCancelWaitFor) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      engine::TaskCancellationBlocker block_cancel;
      return cv.WaitFor(lock, kWaitPeriod);
    });
    has_started_event.WaitForEvent();
    {
      std::unique_lock<engine::Mutex> lock(mutex);
      task.RequestCancel();
      cv.NotifyOne();
    }
    EXPECT_EQ(engine::CvStatus::kNoTimeout, task.Get());
  }
  {
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      engine::TaskCancellationBlocker block_cancel;
      return cv.WaitFor(lock, kWaitPeriod);
    });
    has_started_event.WaitForEvent();
    {
      std::unique_lock<engine::Mutex> lock(mutex);
      task.RequestCancel();
    }
    EXPECT_EQ(engine::CvStatus::kTimeout, task.Get());
  }
}

UTEST(ConditionVariable, BlockedCancelWaitForPredicate) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;
  SpinEvent has_started_event;
  {
    bool flag = false;
    auto task = engine::AsyncNoSpan([&] {
      std::unique_lock<engine::Mutex> lock(mutex);
      has_started_event.Send();
      engine::TaskCancellationBlocker block_cancel;
      return cv.WaitFor(lock, kWaitPeriod, [&] { return flag; });
    });
    has_started_event.WaitForEvent();
    {
      std::unique_lock<engine::Mutex> lock(mutex);
      task.RequestCancel();
      flag = true;
      cv.NotifyOne();
    }
    EXPECT_TRUE(task.Get());
  }
}

// See cancel_test.cpp for more tests related to task deadlines
UTEST(ConditionVariable, CancelledOnTaskDeadline) {
  const auto deadline =
      engine::Deadline::FromDuration(std::chrono::milliseconds{20});
  engine::Mutex mutex;
  engine::ConditionVariable cond_var;

  auto task = engine::CriticalAsyncNoSpan(deadline, [&] {
    std::unique_lock lock(mutex);
    const auto result = cond_var.WaitFor(lock, utest::kMaxTestWaitTime);
    EXPECT_EQ(result, engine::CvStatus::kCancelled);
  });

  UEXPECT_NO_THROW(task.WaitFor(utest::kMaxTestWaitTime / 2));
  EXPECT_EQ(task.GetState(), engine::Task::State::kCompleted);
  UEXPECT_NO_THROW(task.Get());
}

UTEST(ConditionVariable, AlreadyCancelled) {
  engine::Mutex mutex;
  engine::ConditionVariable cv;

  engine::current_task::GetCancellationToken().RequestCancel();

  std::unique_lock lock(mutex);
  UEXPECT_NO_THROW(EXPECT_EQ(cv.WaitFor(lock, utest::kMaxTestWaitTime),
                             engine::CvStatus::kCancelled));
  UEXPECT_NO_THROW(EXPECT_FALSE(
      cv.WaitFor(lock, utest::kMaxTestWaitTime, [] { return false; })));
}

USERVER_NAMESPACE_END
