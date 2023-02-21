#include <userver/utest/utest.hpp>

#include <array>
#include <atomic>
#include <chrono>

#include <userver/engine/async.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

UTEST(WaitAny, VectorTasks) {
  const size_t kTaskCount = 4;
  const size_t kTaskOrderShift = 1;
  std::vector<engine::TaskWithResult<size_t>> tasks;
  std::atomic<size_t> finished_counter{0};
  for (size_t i = 0; i < kTaskCount; i++) {
    tasks.push_back(engine::AsyncNoSpan([&finished_counter, i] {
      size_t order = (i + kTaskCount - kTaskOrderShift) % kTaskCount;
      while (finished_counter < order) engine::Yield();
      return i;
    }));
  }
  std::array<bool, kTaskCount> completed{};
  completed.fill(false);
  for (size_t i = 0; i < kTaskCount; i++) {
    auto task_idx_opt = engine::WaitAny(tasks);
    ASSERT_TRUE(!!task_idx_opt);

    // After calling Get() the task will be ignored by WaitAny()
    auto task_res = tasks[*task_idx_opt].Get();
    EXPECT_EQ(task_res, (finished_counter + kTaskOrderShift) % kTaskCount);
    completed[task_res] = true;
    ++finished_counter;
  }
  for (size_t i = 0; i < kTaskCount; i++) {
    EXPECT_TRUE(completed[i]);
  }
  EXPECT_EQ(engine::WaitAny(tasks), std::nullopt);
}

UTEST(WaitAny, Cancelled) {
  std::atomic<bool> started{false};
  auto task = engine::AsyncNoSpan([&started]() {
    const size_t kTaskCount = 3;
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < kTaskCount; i++) {
      tasks.push_back(engine::AsyncNoSpan([] {
        for (;;) {
          engine::Yield();
          engine::current_task::CancellationPoint();
        }
      }));
    }

    started = true;
    auto task_idx_opt = engine::WaitAny(tasks);
    ASSERT_EQ(task_idx_opt, std::nullopt);
  });
  while (!started.load()) engine::Yield();

  task.SyncCancel();
}

UTEST(WaitAny, WaitAnyFor) {
  engine::TaskWithResult<void> tasks[] = {
      engine::AsyncNoSpan([] {
        for (;;) {
          engine::Yield();
          engine::current_task::CancellationPoint();
        }
      }),
      engine::AsyncNoSpan([] {}),
  };

  engine::Yield();

  auto task_idx_opt1 = engine::WaitAnyFor(utest::kMaxTestWaitTime, tasks);
  ASSERT_NE(task_idx_opt1, std::nullopt);
  ASSERT_EQ(*task_idx_opt1, 1);

  // test call WaitAny without Get for finished task
  ASSERT_EQ(engine::WaitAnyFor(utest::kMaxTestWaitTime, tasks), task_idx_opt1);

  tasks[*task_idx_opt1].Get();

  auto task_idx_opt2 = engine::WaitAnyFor(42ms, tasks);
  ASSERT_EQ(task_idx_opt2, std::nullopt);
}

UTEST(WaitAny, WaitAnyUntil) {
  const size_t kTaskCount = 2;
  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < kTaskCount; i++) {
    tasks.push_back(engine::AsyncNoSpan([i] {
      if (i == 1) {
        engine::SleepFor(10ms);
        return;
      }
      for (;;) {
        engine::Yield();
        engine::current_task::CancellationPoint();
      }
    }));
  }

  engine::Yield();

  auto until = std::chrono::steady_clock::now() + utest::kMaxTestWaitTime;
  auto task_idx_opt1 = engine::WaitAnyUntil(until, tasks);
  ASSERT_NE(task_idx_opt1, std::nullopt);
  ASSERT_EQ(*task_idx_opt1, 1);
  tasks[*task_idx_opt1].Get();

  auto task_idx_opt2 =
      engine::WaitAnyUntil(engine::Deadline::FromDuration(42ms), tasks);
  ASSERT_EQ(task_idx_opt2, std::nullopt);
}

UTEST(WaitAny, DistinctTypes) {
  auto task0 = engine::AsyncNoSpan([] {
    engine::SleepFor(30ms);
    return 1;
  });
  auto task1 = engine::AsyncNoSpan([] {
    engine::SleepFor(10ms);
    return std::string{"abc"};
  });

  int mask = 0;
  for (size_t i = 0; i < 2; i++) {
    auto task_idx_opt =
        engine::WaitAnyFor(utest::kMaxTestWaitTime, task0, task1);
    ASSERT_NE(task_idx_opt, std::nullopt);
    ASSERT_TRUE(*task_idx_opt == 0 || *task_idx_opt == 1);
    mask |= 1 << *task_idx_opt;
    if (*task_idx_opt == 0)
      EXPECT_EQ(task0.Get(), 1);
    else
      EXPECT_EQ(task1.Get(), std::string{"abc"});
  }
  EXPECT_EQ(mask, 3);
}

UTEST(WaitAny, Sample) {
  /// [sample waitany]
  auto task0 = engine::AsyncNoSpan([] { return 1; });

  auto task1 = utils::Async("long_task", [] {
    engine::InterruptibleSleepFor(20s);
    return std::string{"abc"};
  });

  auto task_idx_opt = engine::WaitAny(task0, task1);
  ASSERT_TRUE(task_idx_opt);
  EXPECT_EQ(*task_idx_opt, 0);
  /// [sample waitany]
}

UTEST(WaitAny, Throwing) {
  const size_t kTaskCount = 2;
  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < kTaskCount; i++) {
    tasks.push_back(engine::AsyncNoSpan([i] {
      if (i == 1) throw std::runtime_error("test");
      for (;;) {
        engine::Yield();
        engine::current_task::CancellationPoint();
      }
    }));
  }

  engine::Yield();

  auto task_idx_opt1 = engine::WaitAnyFor(utest::kMaxTestWaitTime, tasks);
  ASSERT_NE(task_idx_opt1, std::nullopt);
  ASSERT_EQ(*task_idx_opt1, 1);
  UEXPECT_THROW(tasks[*task_idx_opt1].Get(), std::runtime_error);

  auto task_idx_opt2 = engine::WaitAnyFor(42ms, tasks);
  ASSERT_EQ(task_idx_opt2, std::nullopt);
}

#ifndef NDEBUG
UTEST_DEATH(WaitAnyDeathTest, DuplicateTask) {
  const size_t kTaskCount = 2;
  std::vector<engine::TaskWithResult<void>> tasks;
  for (size_t i = 0; i < kTaskCount; i++) {
    tasks.push_back(engine::AsyncNoSpan([] { engine::SleepFor(10ms); }));
  }

  UEXPECT_DEATH(engine::WaitAny(tasks[0], tasks[1], tasks[0]), "");
  UEXPECT_DEATH(
      engine::WaitAnyFor(utest::kMaxTestWaitTime, tasks[0], tasks[1], tasks[0]),
      "");
  UEXPECT_DEATH(engine::WaitAnyUntil(engine::Deadline::FromDuration(42ms),
                                     tasks[0], tasks[1], tasks[0]),
                "");
}
#endif

UTEST(WaitAny, InvalidTask) {
  engine::TaskWithResult<void> task;
  EXPECT_EQ(engine::WaitAny(task), std::nullopt);
}

UTEST(WaitAny, NoTasks) {
  EXPECT_EQ(engine::WaitAny(), std::nullopt);
  EXPECT_EQ(engine::WaitAnyFor(utest::kMaxTestWaitTime), std::nullopt);
  EXPECT_EQ(engine::WaitAnyUntil({}), std::nullopt);

  std::vector<engine::TaskWithResult<int>> no_tasks;
  EXPECT_EQ(engine::WaitAny(no_tasks), std::nullopt);
  EXPECT_EQ(engine::WaitAnyFor(utest::kMaxTestWaitTime, no_tasks),
            std::nullopt);
  EXPECT_EQ(engine::WaitAnyUntil({}, no_tasks), std::nullopt);
}

UTEST(WaitAny, HeterogenousWait) {
  constexpr int kExpectedValue = 42;

  auto task = engine::AsyncNoSpan([] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    return kExpectedValue;
  });

  engine::Promise<int> promise;
  auto future = promise.get_future();

  auto notifier_task = engine::AsyncNoSpan([&] {
    engine::SleepFor(20ms);
    promise.set_value(kExpectedValue);
  });

  UEXPECT_NO_THROW(EXPECT_EQ(engine::WaitAny(task, future), 1));

  EXPECT_TRUE(task.IsValid());
  EXPECT_FALSE(task.IsFinished());
  EXPECT_EQ(future.wait_for(0s), engine::FutureStatus::kReady);
  EXPECT_EQ(future.get(), kExpectedValue);
}

USERVER_NAMESPACE_END
