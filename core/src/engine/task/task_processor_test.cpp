#include <engine/task/task_processor.hpp>

#include <engine/task/task_processor_config.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(TaskProcessor, Overload) {
    engine::TaskProcessorSettings settings;
    settings.wait_queue_overload.action = engine::TaskProcessorSettingsOverloadAction::kCancel;
    settings.wait_queue_overload.length_limit = 500;
    constexpr std::size_t kCreatedTasksCount = 1100;
    constexpr std::size_t kMinCanceledTasksCount = 100;
    engine::current_task::GetTaskProcessor().SetSettings(settings, {});

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

UTEST_MT(TaskProcessor, MetricsAliveAndRunning, 2) {
    auto& task_counter = engine::current_task::GetTaskProcessor().GetTaskCounter();

    EXPECT_EQ(task_counter.GetRunningTasks(), 1);

    std::atomic<bool> ready{false};
    auto task = engine::AsyncNoSpan([&ready] {
        ready = true;
        // wait until end of test
        while (ready) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });

    // wait until task is running
    while (!ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    EXPECT_EQ(task_counter.GetRunningTasks(), 2);

    ready = false;
    task.Get();

    // Task decrements `RunningTasks` metric after setting its `IsFinished`.
    while (task_counter.GetRunningTasks() == 2) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_EQ(task_counter.GetRunningTasks(), 1);
}

USERVER_NAMESPACE_END
