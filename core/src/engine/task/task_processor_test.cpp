#include <userver/utest/utest.hpp>

#include <engine/task/task_processor.hpp>
#include <engine/task/task_processor_config.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(TaskProcessor, Overload) {
  engine::TaskProcessorSettings settings;
  settings.overload_action =
      engine::TaskProcessorSettings::OverloadAction::kCancel;
  settings.wait_queue_length_limit = 500;
  const std::size_t kCreatedTasksCount = 1100;
  const std::size_t kMinCanceledTasksCount = 500;
  engine::current_task::GetTaskProcessor().SetSettings(settings);

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(kCreatedTasksCount);

  for (size_t i = 0; i < kCreatedTasksCount; ++i) {
    tasks.push_back(engine::AsyncNoSpan([]() {}));
  }

  std::size_t canceled_tasks_count{0};
  for (auto& task : tasks) {
    task.Wait();
    if (engine::Task::State::kCancelled == task.GetState()) {
      ++canceled_tasks_count;
    }
  }

  EXPECT_GE(canceled_tasks_count, kMinCanceledTasksCount) << "This test has a 0.15% chance of failing";

  for (std::size_t i = 0; i < 10; ++i) {
    auto task = engine::AsyncNoSpan([]() {});
    task.Wait();
    EXPECT_EQ(task.GetState(), engine::Task::State::kCompleted); 
  }
}

USERVER_NAMESPACE_END
