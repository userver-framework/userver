#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/engine/subprocess/child_process.hpp>
#include <userver/engine/subprocess/process_starter.hpp>
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// MAC_COMPAT
#ifdef __APPLE__
const std::string kTestProgram = "/bin/test";
#else
const std::string kTestProgram = "/usr/bin/test";
#endif

}  // namespace

UTEST(Subprocess, True) {
  engine::subprocess::ProcessStarter starter(
      engine::current_task::GetTaskProcessor());

  auto status = starter.Exec(kTestProgram, {"-n", "1"}).Get();
  ASSERT_TRUE(status.IsExited());
  EXPECT_EQ(0, status.GetExitCode());
}

UTEST(Subprocess, False) {
  engine::subprocess::ProcessStarter starter(
      engine::current_task::GetTaskProcessor());

  auto status = starter.Exec(kTestProgram, {"-z", "1"}).Get();
  ASSERT_TRUE(status.IsExited());
  EXPECT_NE(0, status.GetExitCode());
}

USERVER_NAMESPACE_END
