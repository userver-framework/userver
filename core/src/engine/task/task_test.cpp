#include <utest/utest.hpp>

#include <atomic>
#include <chrono>

#include <engine/async.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task.hpp>

TEST(Task, Ctr) { engine::Task task; }

TEST(Task, Wait) {
  RunInCoro([] {
    auto task = engine::Async([] {});
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
    auto task = engine::Async([] {});
    task.WaitFor(std::chrono::seconds(10));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
  });
}

TEST(Task, EarlyCancel) {
  RunInCoro([] {
    auto task =
        engine::Async([] { ADD_FAILURE() << "Cancelled task has started"; });
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
  });
}

TEST(Task, Cancel) {
  RunInCoro([] {
    auto task = engine::Async([] {
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

TEST(Task, CancelWithPoint) {
  RunInCoro([] {
    auto task = engine::Async([] {
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
    auto task = engine::Async([] {
      engine::InterruptibleSleepFor(std::chrono::seconds(10));
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
  });
}

TEST(Task, Get) {
  RunInCoro([] {
    auto result = engine::Async([] { return 12; }).Get();
    EXPECT_EQ(12, result);
  });
}

TEST(Task, GetVoid) {
  RunInCoro([] { EXPECT_NO_THROW(engine::Async([] { return; }).Get()); });
}

TEST(Task, GetException) {
  RunInCoro([] {
    EXPECT_THROW(engine::Async([] { throw std::runtime_error("123"); }).Get(),
                 std::runtime_error);
  });
}

TEST(Task, GetCancel) {
  RunInCoro([] {
    auto task = engine::Async([] {
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
    auto task = engine::Async([] {
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

    auto task = engine::Async([&] {
      auto subtask = engine::Async([&] {
        is_subtask_started = true;
        engine::InterruptibleSleepFor(std::chrono::seconds(10));
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
      });
      EXPECT_THROW(subtask.Wait(), engine::WaitInterruptedException);
    });

    while (!is_subtask_started) engine::Yield();
  });
}
