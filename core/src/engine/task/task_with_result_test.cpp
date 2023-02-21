#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>
#include <memory>
#include <type_traits>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/scope_guard.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TaskWithResult, Ctr) { engine::TaskWithResult<void> task; }

UTEST(TaskWithResult, Wait) {
  auto container = std::vector{1, 2, 3, 4, 5, 6};

  /// [Sample TaskWithResult usage]
  std::vector<engine::TaskWithResult<int>> tasks;
  tasks.reserve(container.size());
  for (auto value : container) {
    // Creating tasks that will be executed in parallel
    tasks.push_back(utils::Async("some_task", [value = std::move(value)] {
      engine::InterruptibleSleepFor(std::chrono::milliseconds(100));
      return value;
    }));
  }
  // we wait for the completion of tasks and get the results
  std::vector<int> results;
  results.reserve(tasks.size());
  for (auto& task : tasks) {
    // If the task completed with an exception,
    // then Get () will throw an exception
    results.push_back(task.Get());
  }
  /// [Sample TaskWithResult usage]

  EXPECT_EQ(container, results);
}

UTEST_DEATH(TaskWithResultDeathTest, NonStdException) {
  // NOLINTNEXTLINE(hicpp-exception-baseclass)
  auto task = engine::AsyncNoSpan([] { throw 42; });
  UEXPECT_DEATH(task.Get(), "not derived from std::exception");
}

UTEST(TaskWithResult, LifetimeIfTaskCancelledBeforeStart) {
  bool is_func_destroyed = false;

  auto on_destruction =
      std::make_unique<utils::ScopeGuard>([&] { is_func_destroyed = true; });
  auto task = engine::AsyncNoSpan(
      [on_destruction = std::move(on_destruction)] { (void)on_destruction; });

  task.SyncCancel();
  EXPECT_TRUE(is_func_destroyed);
}

static_assert(std::is_move_constructible_v<engine::TaskWithResult<void>>);
static_assert(std::is_move_assignable_v<engine::TaskWithResult<void>>);

static_assert(!std::is_copy_assignable_v<engine::TaskWithResult<void>>);
static_assert(!std::is_copy_constructible_v<engine::TaskWithResult<void>>);

USERVER_NAMESPACE_END
