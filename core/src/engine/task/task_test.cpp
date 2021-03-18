#include <utest/utest.hpp>

#include <atomic>
#include <chrono>

#include <engine/async.hpp>
#include <engine/exception.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task.hpp>

TEST(Task, Ctr) { engine::Task task; }

TEST(Task, Wait) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {});
    task.Wait();
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
  });
}

TEST(Task, Yield) {
  RunInCoro([] { engine::Yield(); });
}

TEST(Task, WaitFor) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {});
    task.WaitFor(std::chrono::seconds(10));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
  });
}

TEST(Task, EarlyCancel) {
  RunInCoro([] {
    auto task = engine::impl::Async(
        [] { ADD_FAILURE() << "Cancelled task has started"; });
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
  });
}

TEST(Task, EarlyCancelResourceCleanup) {
  RunInCoro([] {
    auto shared = std::make_shared<int>(1);
    std::weak_ptr<int> weak = shared;

    // Unlike `engine::TaskWithResult` the `engine::Task` frees resources on
    // finish
    engine::Task task = engine::impl::Async([shared = std::move(shared)] {
      ADD_FAILURE() << "Cancelled task has started";
    });

    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_FALSE(weak.lock());
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
  });
}

TEST(Task, EarlyCancelCritical) {
  RunInCoro([] {
    auto task = engine::impl::CriticalAsync([] { return true; });
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
    EXPECT_TRUE(task.Get());
  });
}

TEST(Task, Cancel) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {
      engine::InterruptibleSleepFor(std::chrono::seconds(10));
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
  });
}

TEST(Task, SyncCancel) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {
      engine::InterruptibleSleepFor(std::chrono::seconds(10));
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
    EXPECT_TRUE(task.IsValid());

    task.SyncCancel();
    EXPECT_TRUE(task.IsFinished());
    EXPECT_TRUE(task.IsValid());
    EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());

    task.SyncCancel();
    EXPECT_TRUE(task.IsFinished());
    EXPECT_TRUE(task.IsValid());
    EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
  });
}

TEST(Task, CancelWithPoint) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {
      engine::InterruptibleSleepFor(std::chrono::seconds(10));
      engine::current_task::CancellationPoint();
      ADD_FAILURE() << "Task ran past cancellation point";
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
  });
}

TEST(Task, AutoCancel) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {
      engine::InterruptibleSleepFor(std::chrono::seconds(10));
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
  });
}

TEST(Task, Get) {
  RunInCoro([] {
    auto result = engine::impl::Async([] { return 12; }).Get();
    EXPECT_EQ(12, result);
  });
}

TEST(Task, GetVoid) {
  RunInCoro([] { EXPECT_NO_THROW(engine::impl::Async([] { return; }).Get()); });
}

TEST(Task, GetException) {
  RunInCoro([] {
    EXPECT_THROW(
        engine::impl::Async([] { throw std::runtime_error("123"); }).Get(),
        std::runtime_error);
  });
}

TEST(Task, GetCancel) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {
      engine::InterruptibleSleepFor(std::chrono::seconds(10));
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });
    engine::Yield();
    task.RequestCancel();
    EXPECT_NO_THROW(task.Get());
  });
}

TEST(Task, GetCancelWithPoint) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {
      engine::InterruptibleSleepFor(std::chrono::seconds(10));
      engine::current_task::CancellationPoint();
      ADD_FAILURE() << "Task ran past cancellation point";
    });
    engine::Yield();
    task.RequestCancel();
    EXPECT_THROW(task.Get(), engine::TaskCancelledException);
  });
}

TEST(Task, CancelWaiting) {
  RunInCoro([] {
    std::atomic<bool> is_subtask_started{false};

    auto task = engine::impl::Async([&] {
      auto subtask = engine::impl::Async([&] {
        is_subtask_started = true;
        engine::InterruptibleSleepFor(std::chrono::seconds(10));
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
      });
      EXPECT_THROW(subtask.Wait(), engine::WaitInterruptedException);
    });

    while (!is_subtask_started) engine::Yield();
  });
}

TEST(Task, GetInvalidatesTask) {
  RunInCoro([] {
    auto task = engine::impl::Async([] {});
    ASSERT_TRUE(task.IsValid());
    EXPECT_NO_THROW(task.Get());
    EXPECT_FALSE(task.IsValid());
  });
}

TEST(Task, GetStackSize) {
  static constexpr size_t kMinimalStackSize = 1;

  RunInCoro([] {
    EXPECT_GE(engine::current_task::GetStackSize(), kMinimalStackSize);
  });
}
