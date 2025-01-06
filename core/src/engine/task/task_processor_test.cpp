#include <engine/task/task_processor.hpp>

#include <engine/task/task_processor_config.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(TaskProcessor, Overload) {
    engine::TaskProcessorSettings settings;
    settings.overload_action = engine::TaskProcessorSettings::OverloadAction::kCancel;
    settings.wait_queue_length_limit = 500;
    constexpr std::size_t kCreatedTasksCount = 1100;
    constexpr std::size_t kMinCanceledTasksCount = 100;
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

    // The queue size for checking for overload is updated with a chance of 1/16.
    // Therefore, with a probability of 1 - (15/16)^500 ~= 1 - 1e-14,
    // by the time the last 100 tasks are added, the queue size will be updated
    // and these tasks will be canceled.
    EXPECT_GE(canceled_tasks_count, kMinCanceledTasksCount);

    // Once the queue size goes down, new tasks should stop being cancelled.
    // This is achieved by updating queue size on every Schedule while overloaded.
    for (std::size_t i = 0; i < 10; ++i) {
        auto task = engine::AsyncNoSpan([]() {});
        task.Wait();
        EXPECT_EQ(task.GetState(), engine::Task::State::kCompleted);
    }
}

USERVER_NAMESPACE_END
