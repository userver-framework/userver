#include <userver/utest/utest.hpp>

#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <boost/dll/runtime_symbol_info.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <engine/task/task_processor.hpp>
#include <userver/engine/subprocess/child_process.hpp>
#include <userver/engine/subprocess/process_starter.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/utest/current_process_open_files.hpp>
#include <userver/utils/text.hpp>
#include <utils/check_syscall.hpp>

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

UTEST(Subprocess, CheckSpdlogClosesFds) {
  auto file = fs::blocking::TempFile::Create("/tmp", "spdlog_closeexec_test");
  auto logger = logging::MakeFileLogger("to_file", file.GetPath(),
                                        logging::Format::kTskv);
  LOG_ERROR_TO(logger) << "This must be logged";

  auto pid = utils::CheckSyscall(fork(), "fork");
  if (pid) {
    // in parent thread
    int status = 0;
    utils::CheckSyscall(::waitpid(pid, &status, 0), "waitpid");

    const auto return_code = WEXITSTATUS(status);
    if (return_code == 1) {
      FAIL() << "File descriptor for '" << file.GetPath()
             << "' was inherited from SpdLog. Make sure that spdlog compiled "
                "with -DSPDLOG_PREVENT_CHILD_FD";
    } else if (return_code == 2) {
      FAIL() << "Too many open file descriptors in child";
    }
    EXPECT_EQ(return_code, 0);
  } else {
    // in child thread
    auto self = boost::dll::program_location();
    char* newargv[] = {
        const_cast<char*>(self.c_str()),
        const_cast<char*>("--gtest_also_run_disabled_tests"),
        const_cast<char*>(
            "--gtest_filter=Subprocess.DISABLED_CheckSpdlogClosesFdsFromChild"),
        nullptr,
    };
    char* newenviron[] = {nullptr};
    execve(self.c_str(), newargv, newenviron);
    std::exit(3);
  }
}

TEST(Subprocess, DISABLED_CheckSpdlogClosesFdsFromChild) {
  static const std::string kSpdlogFilePrefix = "/tmp/spdlog_closeexec_test";

  const auto opened_files = utest::CurrentProcessOpenFiles();
  for (const auto& file : opened_files) {
    if (utils::text::StartsWith(file, kSpdlogFilePrefix)) {
      std::exit(1);
    }
  }

  if (opened_files.size() > 4) std::exit(2);
  std::exit(0);
}

USERVER_NAMESPACE_END
