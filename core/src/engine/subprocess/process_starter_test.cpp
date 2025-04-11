#include <userver/utest/utest.hpp>

#include <sys/param.h>
#include <unistd.h>

#include <chrono>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#elif defined(BSD)
#include <sys/sysctl.h>
#include <sys/types.h>
#else
#include <boost/filesystem.hpp>
#endif

#include <gmock/gmock.h>
#include <boost/range/adaptors.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/ev/thread_pool.hpp>
#include <userver/engine/subprocess/child_process.hpp>
#include <userver/engine/subprocess/process_starter.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/fs/blocking/file_descriptor.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_file.hpp>
#include <userver/fs/temp_file.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/utest/current_process_open_files.hpp>
#include <userver/utils/text.hpp>
#include <userver/utils/text_light.hpp>
#include <utils/check_syscall.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

// MAC_COMPAT
#ifdef __APPLE__
const std::string kPath = "/bin";
#elif defined(BSD)
const std::string kPath = "/bin";
#else
const std::string kPath = "/usr/bin";
#endif
const std::string kProgram = "test";
const std::string kTestProgram = kPath + "/" + kProgram;
constexpr std::string_view kLogFilePart = "log_closeexec_test_";

std::optional<std::string>
GetEnvironmentInsideExec(engine::subprocess::ProcessStarter& starter, const std::string& variable_name) {
    auto stdout_file = fs::TempFile::Create(engine::current_task::GetTaskProcessor());

    engine::subprocess::EnvironmentVariablesScope scope{};
    SetEnvironmentVariable("PATH", kPath, engine::subprocess::Overwrite::kAllowed);

    engine::subprocess::ExecOptions options{};
    options.use_path = true;
    options.stdout_file = stdout_file.GetPath();

    auto status = starter.Exec("printenv", {variable_name}, std::move(options)).Get();

    EXPECT_EQ(true, status.IsExited());
    EXPECT_EQ(true, fs::blocking::FileExists(stdout_file.GetPath()));

    if (status.GetExitCode() != 0) {
        return std::nullopt;
    }

    auto data = fs::blocking::ReadFileContents(stdout_file.GetPath());
    data.pop_back();  // Remove trailing \n.
    return data;
}

}  // namespace

UTEST(Subprocess, ExecvExecvFailure) {
    engine::subprocess::ProcessStarter starter(engine::current_task::GetTaskProcessor());
    auto status = starter.Exec(kTestProgram, {"-z", "1"}).Get();
    ASSERT_TRUE(status.IsExited());
    EXPECT_NE(0, status.GetExitCode());
}

UTEST(Subprocess, ExecvFileNotFound) {
    engine::subprocess::ProcessStarter starter(engine::current_task::GetTaskProcessor());
    const auto status = starter.Exec("myawesomebinary", {}).Get();
    ASSERT_FALSE(status.IsExited());
}

UTEST(Subprocess, EnvironmentVariablesScope) {
    engine::subprocess::ProcessStarter starter(engine::current_task::GetTaskProcessor());
    const std::string kEnvVariableName = "SUPER_DUPER";
    const std::string kEnvVariableValue = "secret";

    ASSERT_EQ(std::nullopt, GetEnvironmentInsideExec(starter, kEnvVariableName));

    const auto before = engine::subprocess::GetCurrentEnvironmentVariables();
    {
        engine::subprocess::EnvironmentVariablesScope scope{};
        SetEnvironmentVariable(kEnvVariableName, kEnvVariableValue, engine::subprocess::Overwrite::kAllowed);

        EXPECT_EQ(kEnvVariableValue, engine::subprocess::GetCurrentEnvironmentVariables().GetValue(kEnvVariableName));

        ASSERT_EQ(kEnvVariableValue, GetEnvironmentInsideExec(starter, kEnvVariableName));
    }

    ASSERT_EQ(std::nullopt, GetEnvironmentInsideExec(starter, kEnvVariableName));

    const auto after = engine::subprocess::GetCurrentEnvironmentVariables();
    EXPECT_EQ(after, before);
}

UTEST(Subprocess, ExecvpSuccess) {
    engine::subprocess::ProcessStarter starter(engine::current_task::GetTaskProcessor());

    engine::subprocess::EnvironmentVariablesScope scope{};
    SetEnvironmentVariable("PATH", kPath, engine::subprocess::Overwrite::kAllowed);

    engine::subprocess::ExecOptions options{};
    options.use_path = true;

    auto status = starter.Exec(kProgram, {"-n", "1"}, std::move(options)).Get();
    ASSERT_TRUE(status.IsExited());
    EXPECT_EQ(0, status.GetExitCode());
}

UTEST(Subprocess, ExecvpVulnerability) {
    engine::subprocess::ProcessStarter starter(engine::current_task::GetTaskProcessor());

    engine::subprocess::EnvironmentVariablesScope scope{};
    engine::subprocess::UnsetEnvironmentVariable("PATH");

    engine::subprocess::ExecOptions options{};
    options.use_path = true;

    UEXPECT_THROW_MSG(
        (void)starter.Exec("/mybinary", {"-n", "1"}, std::move(options)).Get(),
        std::runtime_error,
        "execvp potential vulnerability. more details "
        "https://github.com/userver-framework/userver/issues/588"
    );
}

UTEST(Subprocess, CheckLogClosesFds) {
    auto file = fs::blocking::TempFile::Create("/tmp", kLogFilePart);
    auto logger = logging::MakeFileLogger("to_file", file.GetPath(), logging::Format::kTskv);
    LOG_ERROR_TO(logger) << "This must be logged";

    engine::subprocess::ProcessStarter starter(engine::current_task::GetTaskProcessor());

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
            "--gtest_filter=Subprocess.DISABLED_CheckLogClosesFdsFromChild",
        }
    );

    auto status = future.Get();
    ASSERT_TRUE(status.IsExited());
    const auto return_code = status.GetExitCode();
    EXPECT_NE(return_code, 1) << "File descriptor for '" << file.GetPath() << "' was inherited from Log.";
    EXPECT_EQ(return_code, 0);
}

// This test is run from Subprocess.CheckLogClosesFds
TEST(Subprocess, DISABLED_CheckLogClosesFdsFromChild) {
    const auto opened_files = utest::CurrentProcessOpenFiles();
    for (const auto& file : opened_files) {
        // CurrentProcessOpenFiles.Basic makes sure that this works well
        if (file.find(kLogFilePart) != std::string::npos) {
            LOG_ERROR() << "Error: " << file;
            // NOLINTNEXTLINE(concurrency-mt-unsafe)
            std::exit(1);
        }
    }
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    std::exit(0);
}

USERVER_NAMESPACE_END
