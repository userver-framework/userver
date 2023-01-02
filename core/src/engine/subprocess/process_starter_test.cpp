#include <userver/utest/utest.hpp>

#include <sys/param.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(BSD)
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <boost/filesystem.hpp>
#endif

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
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
#elif defined(BSD)
const std::string kTestProgram = "/bin/test";
#else
const std::string kTestProgram = "/usr/bin/test";
#endif

constexpr std::string_view kSpdlogFilePart = "spdlog_closeexec_test_";

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
  auto file = fs::blocking::TempFile::Create("/tmp", kSpdlogFilePart);
  auto logger = logging::MakeFileLogger("to_file", file.GetPath(),
                                        logging::Format::kTskv);
  LOG_ERROR_TO(logger) << "This must be logged";

  engine::subprocess::ProcessStarter starter(
      engine::current_task::GetTaskProcessor());

#if defined(__APPLE__)
  std::string self;
  uint32_t self_len = 0;
  ASSERT_EQ(_NSGetExecutablePath(self.data(), &self_len), -1);
  self.resize(self_len);
  ASSERT_EQ(_NSGetExecutablePath(self.data(), &self_len), 0);
#elif defined(BSD)
  int mib[4];
  mib[0] = CTL_KERN;
  mib[1] = KERN_PROC;
  mib[2] = KERN_PROC_PATHNAME;
  mib[3] = -1;
  char buf[1024];
  size_t cb = sizeof(buf);
  ASSERT_EQ(sysctl(mib, 4, buf, &cb, NULL, 0), 0);

  std::string self(buf);
#else
  auto self = boost::filesystem::read_symlink("/proc/self/exe").native();
#endif

  auto future = starter.Exec(
      self,
      {
          "--gtest_also_run_disabled_tests",
          "--gtest_filter=Subprocess.DISABLED_CheckSpdlogClosesFdsFromChild",
      });

  auto status = future.Get();
  ASSERT_TRUE(status.IsExited());
  const auto return_code = status.GetExitCode();
  EXPECT_NE(return_code, 1)
      << "File descriptor for '" << file.GetPath()
      << "' was inherited from SpdLog. Make sure that spdlog compiled "
         "with -DSPDLOG_PREVENT_CHILD_FD";
  EXPECT_EQ(return_code, 0);
}

// This test is run from Subprocess.CheckSpdlogClosesFds
TEST(Subprocess, DISABLED_CheckSpdlogClosesFdsFromChild) {
  const auto opened_files = utest::CurrentProcessOpenFiles();
  for (const auto& file : opened_files) {
    // CurrentProcessOpenFiles.Basic makes sure that this works well
    if (file.find(kSpdlogFilePart) != std::string::npos) {
      LOG_ERROR() << "Error: " << file;
      // NOLINTNEXTLINE(concurrency-mt-unsafe)
      std::exit(1);
    }
  }
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  std::exit(0);
}

USERVER_NAMESPACE_END
