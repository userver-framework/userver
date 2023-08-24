#include <userver/utest/utest.hpp>

#include <type_traits>

#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

TEST(SharedTaskWithResult, Ctr) {
  engine::SharedTaskWithResult<void> task;
  engine::SharedTaskWithResult<int> task_int;

  static_assert(sizeof(task) == sizeof(engine::SharedTask),
                "Additional fields will be lost by slicing of "
                "SharedTaskWithResult into SharedTask");
  static_assert(sizeof(task_int) == sizeof(engine::SharedTask),
                "Additional fields will be lost by slicing of "
                "SharedTaskWithResult into SharedTask");
}

TEST(SharedTaskWithResult, MoveCtr) {
  engine::SharedTaskWithResult<void> t;
  engine::SharedTaskWithResult<void> t2 = std::move(t);

  using IntSharedTask = engine::SharedTaskWithResult<int>;
  IntSharedTask t_int;
  IntSharedTask t2_int = std::move(t_int);
}

UTEST(SharedTaskWithResult, MoveInvalidatesOther) {
  auto moved_from = utils::SharedAsync("we", []() {});
  EXPECT_TRUE(moved_from.IsValid());

  engine::SharedTaskWithResult<void> moved_to{std::move(moved_from)};
  EXPECT_TRUE(moved_to.IsValid());
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move,bugprone-use-after-move)
  EXPECT_FALSE(moved_from.IsValid());
}

TEST(SharedTaskWithResult, CopyCtr) {
  engine::SharedTaskWithResult<void> t;
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  engine::SharedTaskWithResult<void> t2 = t;

  engine::SharedTaskWithResult<int> t_int;
  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  engine::SharedTaskWithResult<int> t2_int = t_int;
}

UTEST(SharedTaskWithResult, CopyLeavesBothTasksValid) {
  auto copy_from =
      utils::SharedAsync("we", []() { return std::make_unique<int>(5); });
  EXPECT_TRUE(copy_from.IsValid());

  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  auto copy_to = copy_from;
  EXPECT_TRUE(copy_from.IsValid());
  EXPECT_TRUE(copy_to.IsValid());

  EXPECT_EQ(*copy_from.Get(), 5);
  EXPECT_EQ(*copy_to.Get(), 5);
}

UTEST(SharedTaskWithResult, DestroyOfCopyLeavesInitialTaskValid) {
  auto copy_from =
      utils::SharedAsync("we", []() { return std::make_unique<int>(5); });
  EXPECT_TRUE(copy_from.IsValid());

  {
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    auto copy_to = copy_from;
    EXPECT_TRUE(copy_from.IsValid());
    EXPECT_TRUE(copy_to.IsValid());
  }

  EXPECT_TRUE(copy_from.IsValid());
  EXPECT_EQ(*copy_from.Get(), 5);
}

UTEST(SharedTaskWithResult, MoveToCopyLeavesInitialTaskValid) {
  auto copy_from =
      utils::SharedAsync("we", []() { return std::make_unique<int>(5); });
  EXPECT_TRUE(copy_from.IsValid());

  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  auto copy_to = copy_from;
  EXPECT_TRUE(copy_from.IsValid());
  EXPECT_TRUE(copy_to.IsValid());

  copy_to =
      utils::SharedAsync("we", []() { return std::make_unique<int>(42); });

  EXPECT_TRUE(copy_from.IsValid());
  EXPECT_EQ(*copy_from.Get(), 5);

  EXPECT_TRUE(copy_to.IsValid());
  EXPECT_EQ(*copy_to.Get(), 42);
}

UTEST(SharedTaskWithResult, AssignToCopyLeavesInitialTaskValid) {
  auto copy_from =
      utils::SharedAsync("we", []() { return std::make_unique<int>(5); });
  EXPECT_TRUE(copy_from.IsValid());

  auto copy_to = copy_from;
  EXPECT_TRUE(copy_from.IsValid());
  EXPECT_TRUE(copy_to.IsValid());

  auto another_copy_from =
      utils::SharedAsync("we", []() { return std::make_unique<int>(42); });
  copy_to = another_copy_from;

  EXPECT_TRUE(copy_from.IsValid());
  EXPECT_EQ(*copy_from.Get(), 5);

  EXPECT_TRUE(copy_to.IsValid());
  EXPECT_EQ(*copy_to.Get(), 42);

  EXPECT_TRUE(another_copy_from.IsValid());
  EXPECT_EQ(*another_copy_from.Get(), 42);
}

UTEST(SharedTaskWithResult, CopyAfterCompletion) {
  auto copy_from =
      utils::SharedAsync("we", []() { return std::make_unique<int>(5); });
  copy_from.Get();

  // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
  auto copy_to = copy_from;
  EXPECT_EQ(*copy_from.Get(), 5);
  EXPECT_EQ(*copy_to.Get(), 5);
}

UTEST(SharedTaskWithResult, AutoCancel) {
  bool initial_task_was_canceled = false;
  {
    auto task = utils::SharedAsync("cancel me", [&initial_task_was_canceled] {
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

UTEST(SharedTaskWithResult, AutoCancelOnAssignInvalid) {
  bool initial_task_was_canceled = false;

  auto task = utils::SharedAsync("cancel me", [&initial_task_was_canceled] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
    initial_task_was_canceled = engine::current_task::IsCancelRequested();
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
  EXPECT_FALSE(initial_task_was_canceled);

  {
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    auto other = task;
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    task = other;  // self assignment should not invalidate
    engine::Yield();
    EXPECT_FALSE(other.IsFinished());
    EXPECT_FALSE(task.IsFinished());
    EXPECT_FALSE(initial_task_was_canceled);
  }
  EXPECT_FALSE(task.IsFinished());
  EXPECT_FALSE(initial_task_was_canceled);

  {
    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    auto other = task;
    task = std::move(other);  // self move assignment should not invalidate
    engine::Yield();
    // NOLINTNEXTLINE(clang-analyzer-cplusplus.Move)
    EXPECT_FALSE(other.IsFinished());
    EXPECT_FALSE(task.IsFinished());
    EXPECT_FALSE(initial_task_was_canceled);
  }
  EXPECT_FALSE(task.IsFinished());
  EXPECT_FALSE(initial_task_was_canceled);

  task = {};
  EXPECT_FALSE(task.IsValid());
  EXPECT_TRUE(initial_task_was_canceled);
}

UTEST(SharedTaskWithResult, AutoCancelWithCopy) {
  bool initial_task_was_canceled = false;
  {
    auto task = utils::SharedAsync("cancel me", [&initial_task_was_canceled] {
      engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
      initial_task_was_canceled = engine::current_task::IsCancelRequested();
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
    EXPECT_FALSE(initial_task_was_canceled);

    {
      // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
      auto copy_to = task;
      EXPECT_TRUE(task.IsValid());
      EXPECT_TRUE(copy_to.IsValid());
      EXPECT_FALSE(initial_task_was_canceled);
    }

    EXPECT_TRUE(task.IsValid());
    EXPECT_FALSE(initial_task_was_canceled);
  }
  EXPECT_TRUE(initial_task_was_canceled);
}

UTEST(SharedTaskWithResult, AutoCancelWithCopyOutlivesInitialValue) {
  bool initial_task_was_canceled = false;
  {
    auto task = utils::SharedAsync("cancel me", [&initial_task_was_canceled] {
      engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
      EXPECT_TRUE(engine::current_task::IsCancelRequested());
      initial_task_was_canceled = engine::current_task::IsCancelRequested();
    });
    engine::Yield();
    EXPECT_FALSE(task.IsFinished());
    EXPECT_FALSE(initial_task_was_canceled);

    // NOLINTNEXTLINE(performance-unnecessary-copy-initialization)
    auto copy_to = task;
    EXPECT_TRUE(task.IsValid());
    EXPECT_TRUE(copy_to.IsValid());
    EXPECT_FALSE(initial_task_was_canceled);

    task = utils::SharedAsync("cancel me 2", []() {});
    engine::Yield();
    EXPECT_TRUE(copy_to.IsValid());
    EXPECT_FALSE(initial_task_was_canceled);
  }
  EXPECT_TRUE(initial_task_was_canceled);
}

UTEST(SharedTaskWithResult, AutoCancelOnMove) {
  bool initial_task_was_cancelled = false;
  auto task = utils::SharedAsync("cancel me", [&initial_task_was_cancelled] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
    initial_task_was_cancelled = engine::current_task::IsCancelRequested();
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
  EXPECT_FALSE(initial_task_was_cancelled);

  bool was_invoked = false;
  task = utils::SharedAsync("new", [&was_invoked] { was_invoked = true; });
  EXPECT_TRUE(initial_task_was_cancelled);
  EXPECT_EQ(was_invoked, task.IsFinished());
  EXPECT_TRUE(task.IsValid());
  engine::Yield();
  EXPECT_TRUE(was_invoked);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_TRUE(task.IsValid());
}

UTEST(SharedTaskWithResult, AutoCancelOnAssign) {
  bool initial_task_was_cancelled = false;
  auto task = utils::SharedAsync("cancel me", [&initial_task_was_cancelled] {
    engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(engine::current_task::IsCancelRequested());
    initial_task_was_cancelled = engine::current_task::IsCancelRequested();
  });
  engine::Yield();
  EXPECT_FALSE(task.IsFinished());
  EXPECT_FALSE(initial_task_was_cancelled);

  bool was_invoked = false;
  auto new_task =
      utils::SharedAsync("new", [&was_invoked] { was_invoked = true; });
  task = new_task;
  EXPECT_TRUE(initial_task_was_cancelled);
  EXPECT_EQ(was_invoked, task.IsFinished());
  EXPECT_TRUE(task.IsValid());
  engine::Yield();
  EXPECT_TRUE(was_invoked);
  EXPECT_TRUE(task.IsFinished());
  EXPECT_TRUE(task.IsValid());
}

UTEST(SharedTaskWithResult, Wait) {
  auto task = utils::SharedAsync("we", []() { return 5; });

  task.Wait();
}

UTEST(SharedTaskWithResult, Get) {
  auto task = utils::SharedAsync("we", []() {});

  task.Get();
}

UTEST(SharedTaskWithResult, GetMultiThrow) {
  auto task =
      utils::SharedAsync("we", []() { throw std::runtime_error{"error"}; });

  UEXPECT_THROW(task.Get(), std::runtime_error);
  UEXPECT_THROW(task.Get(), std::runtime_error);
}

UTEST(SharedTaskWithResult, WaitNoThrow) {
  auto task =
      utils::SharedAsync("we", []() { throw std::runtime_error{"error"}; });

  UEXPECT_NO_THROW(task.Wait());
}

UTEST_MT(SharedTaskWithResult, MultithreadedGet, 4) {
  /// [Sample SharedTaskWithResult usage]
  // Creating task that will be awaited in parallel
  auto shared_task = utils::SharedAsync("shared_task", []() {
    engine::InterruptibleSleepFor(std::chrono::milliseconds(100));

    return 1;
  });

  std::vector<engine::TaskWithResult<void>> tasks;
  std::atomic_int sum{0};
  for (size_t i = 0; i < 4; ++i) {
    // Getting shared_task's result in parallel
    tasks.push_back(
        utils::Async("waiter_task", [shared_task /* copy is allowed */,
                                     &sum]() { sum += shared_task.Get(); }));
  }

  for (auto& task : tasks) task.Wait();
  /// [Sample SharedTaskWithResult usage]

  EXPECT_EQ(sum, 4);
}

UTEST_MT(SharedTaskWithResult, MultithreadedWait, 4) {
  auto shared_task = utils::SharedAsync("we", []() {
    engine::SingleConsumerEvent event;
    EXPECT_FALSE(event.WaitForEventFor(std::chrono::milliseconds{50}));

    return 1;
  });

  std::vector<engine::TaskWithResult<void>> tasks;
  std::atomic_int sum{0};
  for (size_t i = 0; i < 4; ++i) {
    tasks.push_back(utils::Async("waiter_task", [&shared_task, &sum]() {
      shared_task.Wait();

      sum += shared_task.Get();
    }));
  }

  for (auto& task : tasks) task.Wait();

  EXPECT_EQ(sum, 4);
}

UTEST(SharedTaskWithResult, MultiWaitGet) {
  const auto res = 2;

  auto task = utils::SharedAsync("we", []() {
    engine::SingleConsumerEvent event;
    EXPECT_FALSE(event.WaitForEventFor(std::chrono::milliseconds{20}));

    return res;
  });

  EXPECT_EQ(task.Get(), res);
  task.Wait();

  EXPECT_EQ(task.Get(), res);
  task.Wait();
}

static_assert(std::is_move_constructible_v<engine::SharedTaskWithResult<void>>);
static_assert(std::is_move_assignable_v<engine::SharedTaskWithResult<void>>);

static_assert(std::is_copy_constructible_v<engine::SharedTaskWithResult<void>>);
static_assert(std::is_copy_assignable_v<engine::SharedTaskWithResult<void>>);

USERVER_NAMESPACE_END
