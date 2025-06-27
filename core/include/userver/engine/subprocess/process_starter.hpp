#pragma once

/// @file userver/engine/subprocess/process_starter.hpp
/// @brief @copybrief engine::subprocess::ProcessStarter

#include <optional>
#include <string>
#include <vector>

#include <userver/engine/subprocess/child_process.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadControl;
}  // namespace engine::ev

namespace engine::subprocess {

/// @brief Structure of settings for executing commands in the OS.
struct ExecOptions final {
    /// EnvironmentVariables for the environment in execution, or `std::nullopt`
    /// to use GetCurrentEnvironmentVariables()
    std::optional<EnvironmentVariables> env{};
    /// EnvironmentVariablesUpdate for the update environment before execution, or
    /// `std::nullopt` to leave `env` as is
    std::optional<EnvironmentVariablesUpdate> env_update{};
    /// File path to be redirected stdout, or `std::nullopt` to use the service's
    /// stdout
    std::optional<std::string> stdout_file{};
    /// File path to be redirected stderr, or `std::nullopt` to use the service's
    /// stderr
    std::optional<std::string> stderr_file{};
    /// If `false`, `executable_path` is treated as absolute path or a relative path.
    /// If `true`, and `executable_path` does not contain `/`, and PATH in environment
    /// variables, then it will be searched in the colon-separated list of
    /// directory pathnames specified in the PATH environment variable.
    /// If `true`, and `executable_path` contains `/`, `executable_path` is treated as absolute
    /// path or a relative path.
    bool use_path{false};
};

/// @ingroup userver_clients
///
/// @brief Creates a new OS subprocess and executes a command in it.
class ProcessStarter {
public:
    /// @param task_processor will be used for executing asynchronous fork + exec.
    /// `main-task-processor` is OK for this purpose.
    explicit ProcessStarter(TaskProcessor& task_processor);

    /// @param executable_path the absolute path or relative path. If `use_path` is
    /// `true`, and `executable_path` does not contain `/`, then it will be searched in
    /// the colon-separated list of directory pathnames specified in the PATH
    /// environment variable. More details @ref ExecOptions::use_path
    /// @param args exact args passed to the executable
    /// @param options @ref ExecOptions settings
    /// @throws std::runtime_error if `use_path` is `true`, `executable_path` contains `/`
    /// and PATH not in environment variables
    ChildProcess
    Exec(const std::string& executable_path, const std::vector<std::string>& args, ExecOptions&& options = {});

private:
    ev::ThreadControl& thread_control_;
};

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
