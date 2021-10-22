#include <userver/utest/utest.hpp>

#include <atomic>
#include <chrono>

#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

TEST(TaskWithResult, Ctr) { engine::TaskWithResult<void> task; }

UTEST(TaskWithResult, Wait) {
  auto container = std::vector{1, 2, 3, 4, 5, 6};

  /// [Sample TaskWithResult usage]
  std::vector<engine::TaskWithResult<int>> tasks;
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

USERVER_NAMESPACE_END
