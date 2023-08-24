#include <userver/utest/utest.hpp>

#include <chrono>

#include <userver/testsuite/periodic_task_control.hpp>
#include <userver/utils/periodic_task.hpp>

USERVER_NAMESPACE_BEGIN

UTEST_DEATH(PeriodicTaskControlDeathTest, Smoke) {
  testsuite::PeriodicTaskControl periodic_task_control;

  size_t task_runs = 0;
  utils::PeriodicTask task("test",
                           utils::PeriodicTask::Settings(std::chrono::hours{1}),
                           [&] { ++task_runs; });
  task.RegisterInTestsuite(periodic_task_control);

  EXPECT_EQ(0, task_runs);
  periodic_task_control.RunPeriodicTask("test");
  EXPECT_EQ(1, task_runs);
  task.Stop();

  EXPECT_UINVARIANT_FAILURE(
      periodic_task_control.RunPeriodicTask("nonexistent"));
}

USERVER_NAMESPACE_END
