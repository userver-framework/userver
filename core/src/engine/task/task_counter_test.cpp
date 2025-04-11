#include <engine/task/task_counter.hpp>

#include <engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task_base.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

// NOTE: these tests use some knowledge of the scheduler that is not a public contract.
// These tests serve as sanity checks for the developers of userver schedulers.
// If the scheduler changes, these tests may have to change as well.

UTEST(TaskCounter, Works) {
    ASSERT_EQ(GetThreadCount(), 1) << "This test assumes that only 1 task can run at a time";

    auto& counter = engine::current_task::GetTaskProcessor().GetTaskCounter();
    EXPECT_EQ(counter.GetCreatedTasks(), 1);
    EXPECT_EQ(counter.GetDestroyedTasks(), 0);
    EXPECT_EQ(counter.GetCancelledTasks(), 0);
    EXPECT_EQ(counter.GetCancelledTasksOverload(), 0);
    EXPECT_EQ(counter.GetTasksOverload(), 0);
    EXPECT_EQ(counter.GetTasksOverloadSensor(), 0);
    EXPECT_EQ(counter.GetTasksStartedRunning(), 1);
    EXPECT_EQ(counter.GetRunningTasks(), 1);

    engine::Yield();

    EXPECT_EQ(counter.GetTasksStartedRunning(), 2);

    auto task = engine::CriticalAsyncNoSpan([] { engine::InterruptibleSleepFor(utest::kMaxTestWaitTime); });

    // `task` has not had the chance to run yet. Still, it is considered as "created".
    EXPECT_EQ(counter.GetCreatedTasks(), 2);

    task.RequestCancel();
    task.Wait();
    EXPECT_TRUE(task.IsValid());

    EXPECT_EQ(counter.GetCreatedTasks(), 2);
    // Because `task.Get()` has not been called yet, it still holds the task to allow waiting on it (which is a noop).
    EXPECT_EQ(counter.GetDestroyedTasks(), 0);
    EXPECT_EQ(counter.GetCancelledTasks(), 1);
    EXPECT_EQ(counter.GetCancelledTasksOverload(), 0);
    EXPECT_EQ(counter.GetTasksOverload(), 0);
    EXPECT_EQ(counter.GetTasksOverloadSensor(), 0);
    // `task` had to sleep before finishing.
    EXPECT_EQ(counter.GetTasksStartedRunning(), 4);
    EXPECT_EQ(counter.GetRunningTasks(), 1);

    // Despite being cancelled, the task has had a chance to run and return normally (guaranteed due to Critical).
    UEXPECT_NO_THROW(task.Get());
    EXPECT_FALSE(task.IsValid());

    EXPECT_EQ(counter.GetCreatedTasks(), 2);
    EXPECT_EQ(counter.GetDestroyedTasks(), 1);
    EXPECT_EQ(counter.GetCancelledTasks(), 1);
    EXPECT_EQ(counter.GetTasksStartedRunning(), 4);
    EXPECT_EQ(counter.GetRunningTasks(), 1);
}

USERVER_NAMESPACE_END
