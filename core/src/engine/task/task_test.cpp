#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>

#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

TEST(Task, Ctr) { engine::Task task; }

UTEST(Task, Wait) {
  auto task = engine::AsyncNoSpan([] {});
  task.Wait();
  EXPECT_TRUE(task.IsFinished());
  EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
}

UTEST(Task, Yield) { engine::Yield(); }

UTEST(Task, WaitFor) {
  auto task = engine::AsyncNoSpan([] {});
  task.WaitFor(kMaxTestWaitTime);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
}
UTEST(Task, EarlyCancel) {
  auto task = engine::AsyncNoSpan(
      [] { ADD_FAILURE() << "Cancelled task has started"; });
  task.RequestCancel();
  task.WaitFor(100ms);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
}

UTEST(Task, EarlyCancelResourceCleanup) {
  auto shared = std::make_shared<int>(1);
  std::weak_ptr<int> weak = shared;

  // Unlike `engine::TaskWithResult` the `engine::Task` frees resources on
  // finish
  engine::Task task = engine::AsyncNoSpan([shared = std::move(shared)] {
    ADD_FAILURE() << "Cancelled task has started";
  });

  task.RequestCancel();
  task.WaitFor(100ms);
  EXPECT_FALSE(weak.lock());
  EXPECT_TRUE(task.IsFinished());
  EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
}

UTEST(Task, EarlyCancelCritical) {
  auto task = engine::CriticalAsyncNoSpan([] { return true; });
  task.RequestCancel();
  task.WaitFor(100ms);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
  EXPECT_TRUE(task.Get());
}

UTEST(Task, Cancel) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
  task.RequestCancel();
  task.WaitFor(100ms);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_EQ(engine::Task::State::kCompleted, task.GetState());
}

UTEST(Task, SyncCancel) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(kMaxTestWaitTime);
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
}

UTEST(Task, CancelWithPoint) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(kMaxTestWaitTime);
    engine::current_task::CancellationPoint();
    ADD_FAILURE() << "Task ran past cancellation point";
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
  task.RequestCancel();
  task.WaitFor(100ms);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_EQ(engine::Task::State::kCancelled, task.GetState());
}

UTEST(Task, AutoCancel) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
}

UTEST(Task, Get) {
  auto result = engine::AsyncNoSpan([] { return 12; }).Get();
  EXPECT_EQ(12, result);
}

UTEST(Task, GetVoid) {
  EXPECT_NO_THROW(engine::AsyncNoSpan([] { return; }).Get());
}

UTEST(Task, GetException) {
  EXPECT_THROW(
      engine::AsyncNoSpan([] { throw std::runtime_error("123"); }).Get(),
      std::runtime_error);
}

UTEST(Task, GetCancel) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
  });
  engine::Yield();
  task.RequestCancel();
  EXPECT_NO_THROW(task.Get());
}

UTEST(Task, GetCancelWithPoint) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(kMaxTestWaitTime);
    engine::current_task::CancellationPoint();
    ADD_FAILURE() << "Task ran past cancellation point";
  });
  engine::Yield();
  task.RequestCancel();
  EXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(Task, CancelWaiting) {
  std::atomic<bool> is_subtask_started{false};

  auto task = engine::AsyncNoSpan([&] {
    auto subtask = engine::AsyncNoSpan([&] {
      is_subtask_started = true;
      engine::InterruptibleSleepFor(kMaxTestWaitTime);
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });
    EXPECT_THROW(subtask.Wait(), engine::WaitInterruptedException);
  });

  while (!is_subtask_started) engine::Yield();
}

UTEST(Task, GetInvalidatesTask) {
  auto task = engine::AsyncNoSpan([] {});
  ASSERT_TRUE(task.IsValid());
  EXPECT_NO_THROW(task.Get());
  EXPECT_FALSE(task.IsValid());
}

UTEST(Task, GetStackSize) {
  static constexpr size_t kMinimalStackSize = 1;

  EXPECT_GE(engine::current_task::GetStackSize(), kMinimalStackSize);
}

USERVER_NAMESPACE_END
