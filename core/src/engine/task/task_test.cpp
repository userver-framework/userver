#include <userver/utest/utest.hpp>

#include <array>
#include <atomic>
#include <chrono>

#include <userver/engine/async.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/single_consumer_event.hpp>
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
  task.WaitFor(utest::kMaxTestWaitTime);
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
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
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
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
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
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
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
  bool initial_task_was_canceled = false;
  {
    auto task = engine::AsyncNoSpan([&initial_task_was_canceled] {
      engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
      initial_task_was_canceled = engine::current_task::IsCancelRequested();
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
    EXPECT_FALSE(initial_task_was_canceled);
  }
  EXPECT_TRUE(initial_task_was_canceled);
}

UTEST(Task, AutoCancelOnAssignInvalid) {
  bool initial_task_was_canceled = false;

  auto task = engine::AsyncNoSpan([&initial_task_was_canceled] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
    initial_task_was_canceled = engine::current_task::IsCancelRequested();
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
  EXPECT_FALSE(initial_task_was_canceled);

  task = {};
  EXPECT_FALSE(task.IsValid());
  EXPECT_TRUE(initial_task_was_canceled);
}

UTEST(Task, AutoCancelOnMoveAssign) {
  bool initial_task_was_cancelled = false;
  auto task = engine::AsyncNoSpan([&initial_task_was_cancelled] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
    initial_task_was_cancelled = engine::current_task::IsCancelRequested();
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
  EXPECT_FALSE(initial_task_was_cancelled);

  bool was_invoked = false;
  task = engine::AsyncNoSpan([&was_invoked] { was_invoked = true; });
  EXPECT_TRUE(initial_task_was_cancelled);
  EXPECT_EQ(was_invoked, task.IsFinished());
  EXPECT_TRUE(task.IsValid());
  engine::Yield();
  EXPECT_TRUE(was_invoked);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_TRUE(task.IsValid());
}

UTEST(Task, MoveConstructor) {
  bool initial_task_was_cancelled = false;
  {
    auto task = engine::AsyncNoSpan([&initial_task_was_cancelled] {
      engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
      initial_task_was_cancelled = engine::current_task::IsCancelRequested();
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
    EXPECT_FALSE(initial_task_was_cancelled);

    auto task_new = std::move(task);
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    EXPECT_FALSE(task.IsValid());
    EXPECT_TRUE(task_new.IsValid());
    EXPECT_FALSE(initial_task_was_cancelled);
  }
  EXPECT_TRUE(initial_task_was_cancelled);
}

UTEST(Task, Get) {
  auto result = engine::AsyncNoSpan([] { return 12; }).Get();
  EXPECT_EQ(12, result);
}

UTEST(Task, GetVoid) {
  UEXPECT_NO_THROW(engine::AsyncNoSpan([] { return; }).Get());
}

UTEST(Task, GetException) {
  UEXPECT_THROW(
      engine::AsyncNoSpan([] { throw std::runtime_error("123"); }).Get(),
      std::runtime_error);
}

UTEST(Task, GetCancel) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
  });
  engine::Yield();
  task.RequestCancel();
  UEXPECT_NO_THROW(task.Get());
}

UTEST(Task, GetCancelWithPoint) {
  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    engine::current_task::CancellationPoint();
    ADD_FAILURE() << "Task ran past cancellation point";
  });
  engine::Yield();
  task.RequestCancel();
  UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(Task, CancelWaiting) {
  std::atomic<bool> is_subtask_started{false};

  auto task = engine::AsyncNoSpan([&] {
    auto subtask = engine::AsyncNoSpan([&] {
      is_subtask_started = true;
      engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });
    UEXPECT_THROW(subtask.Wait(), engine::WaitInterruptedException);
  });

  while (!is_subtask_started) engine::Yield();
}

UTEST(Task, GetInvalidatesTask) {
  auto task = engine::AsyncNoSpan([] {});
  ASSERT_TRUE(task.IsValid());
  UEXPECT_NO_THROW(task.Get());
  EXPECT_FALSE(task.IsValid());
}

UTEST(Task, GetStackSize) {
  static constexpr size_t kMinimalStackSize = 1;

  EXPECT_GE(engine::current_task::GetStackSize(), kMinimalStackSize);
}

UTEST_MT(Task, MultiWait, 4) {
  constexpr size_t kWaitingTasksCount = 4;
  const auto test_deadline =
      engine::Deadline::FromDuration(utest::kMaxTestWaitTime);

  engine::SingleConsumerEvent event;
  auto shared_task = engine::SharedAsyncNoSpan([&event, test_deadline] {
    EXPECT_TRUE(event.WaitForEventUntil(test_deadline));
  });

  engine::Mutex mutex;
  engine::ConditionVariable cv;
  size_t tasks_started = 0;

  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < kWaitingTasksCount; ++i) {
    tasks.push_back(engine::AsyncNoSpan(
        [&shared_task, &mutex, &cv, &tasks_started, test_deadline]() {
          {
            std::unique_lock<engine::Mutex> lock{mutex};
            tasks_started++;
            cv.NotifyOne();
          }

          shared_task.WaitUntil(test_deadline);
        }));
  }

  std::unique_lock<engine::Mutex> lock{mutex};
  ASSERT_TRUE(cv.WaitUntil(lock, test_deadline, [&tasks_started] {
    return tasks_started == kWaitingTasksCount;
  }));
  event.Send();

  for (auto& task : tasks) task.Get();
}

TEST(Task, CoroStackSizeSet) {
  engine::TaskProcessorPoolsConfig config{};
  config.coro_stack_size = 256 * 1024 + 123;
  engine::RunStandalone(1, config, []() {
    ASSERT_EQ(engine::current_task::GetStackSize(), 256 * 1024 + 123);
  });
}

// ASAN has issues with stacks of more than ~4MB, so we use 3MB stacks here
TEST(Task, UseMediumStack) {
  engine::TaskProcessorPoolsConfig config{};
  config.coro_stack_size = 3 * 1024 * 1024ULL;
  engine::RunStandalone(1, config, []() {
    std::array<volatile unsigned char, 1024 * 1024ULL> dummy_data{};
    for (auto& byte : dummy_data) {
      byte = 42;
    }
  });
}

// Flaky due to the https://github.com/llvm/llvm-project/issues/54493
TEST(Task, DISABLED_UseLargeStack) {
  engine::TaskProcessorPoolsConfig config{};
  config.coro_stack_size = 8 * 1024 * 1024ULL;
  engine::RunStandalone(1, config, []() {
    std::array<volatile unsigned char, 7 * 1024 * 1024ULL> dummy_data{};
    for (auto& byte : dummy_data) {
      byte = 12;
    }
  });
}

// This test doesn't really test anything on its own, but shows how bad locking
// in glibc dl_iterate_phdr is when multiple threads are
// unwinding simultaneously.
//
// sudo perf stat -e 'syscalls:sys_enter_futex' ./userver-core_unittest \
//     --gtest_filter='Task.ExceptionStorm'
//
// We use this manually to validate that our caching override
// of dl_iterate_phdr (see exception_hacks.cpp) fixes the issue.
UTEST_MT(Task, ExceptionStorm, 8) {
  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(8);

  for (std::size_t i = 0; i < 8; ++i) {
    tasks.push_back(engine::AsyncNoSpan([] {
      for (std::size_t i = 0; i < 1'000; ++i) {
        try {
          throw std::runtime_error{"42"};
        } catch (const std::exception&) {
        }
      }
    }));
  }

  for (auto& task : tasks) {
    task.Get();
  }
}

USERVER_NAMESPACE_END
