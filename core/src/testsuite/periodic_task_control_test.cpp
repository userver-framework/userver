#include <userver/utest/utest.hpp>

#include <chrono>

#include <userver/testsuite/periodic_task_control.hpp>
#include <userver/utils/periodic_task.hpp>

UTEST(PeriodicTaskControl, Smoke) {
  testing::FLAGS_gtest_death_test_style = "threadsafe";

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

  EXPECT_YTX_INVARIANT_FAILURE(
      periodic_task_control.RunPeriodicTask("nonexistent"));
}
