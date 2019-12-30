#include <utest/utest.hpp>

#include <chrono>

#include <testsuite/periodic_task_control.hpp>
#include <utils/periodic_task.hpp>

TEST(PeriodicTaskControl, Smoke) {
  RunInCoro([] {
    testsuite::PeriodicTaskControl periodic_task_control;

    size_t task_runs = 0;
    utils::PeriodicTask task(
        "test", utils::PeriodicTask::Settings(std::chrono::hours{1}),
        [&] { ++task_runs; });
    task.RegisterInTestsuite(periodic_task_control);

    EXPECT_EQ(0, task_runs);
    periodic_task_control.RunPeriodicTask("test");
    EXPECT_EQ(1, task_runs);
    task.Stop();

    EXPECT_ANY_THROW(periodic_task_control.RunPeriodicTask("nonexistent"));
  });
}
