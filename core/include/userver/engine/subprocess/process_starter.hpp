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
    /// If `false`, `command` is treated as absolute path or a relative path.
    /// If `true`, and `command` does not contain `/`, and PATH in environment
    /// variables, then it will be searched in the colon-separated list of
    /// directory pathnames specified in the PATH environment variable.
    /// If `true`, and `command` contains `/`, `command` is treated as absolute
    /// path or a relative path.
    bool use_path{false};
};

/// @ingroup userver_clients
///
/// @brief Creates a new OS subprocess and executes a command in it.
class ProcessStarter {
public:
    /// @param task_processor will be used for executing asynchronous fork + exec.
    /// `main-task-processor is OK for this purpose.
    explicit ProcessStarter(TaskProcessor& task_processor);

    /// @param command the absolute path or relative path. If `use_path` is
    /// `true`, and `command` does not contains `/`, then it will be searched in
    /// the colon-separated list of directory pathnames specified in the PATH
    /// environment variable. More details @ref ExecOptions::use_path
    /// @param args exact args passed to the executable
    /// @param options @ref ExecOptions settings
    /// @throws std::runtime_error if `use_path` is `true`, `command` contains `/`
    /// and PATH not in environment variables
    ChildProcess Exec(const std::string& command, const std::vector<std::string>& args, ExecOptions&& options = {});

    /// @overload
    /// @param command the absolute path or relative path
    /// @param args exact args passed to the executable
    /// @param env redefines all environment variables
    /// @deprecated Use the `Exec` overload taking @ref ExecOptions
    ChildProcess Exec(
        const std::string& command,
        const std::vector<std::string>& args,
        const EnvironmentVariables& env,
        // TODO: use something like pipes instead of path to files
        const std::optional<std::string>& stdout_file = std::nullopt,
        const std::optional<std::string>& stderr_file = std::nullopt
    );

    /// @overload
    /// @param command the absolute path or relative path
    /// @param args exact args passed to the executable
    /// @param env_update variables to add to the current environment, overwriting
    /// existing ones
    /// @deprecated Use the `Exec` overload taking @ref ExecOptions
    ChildProcess Exec(
        const std::string& command,
        const std::vector<std::string>& args,
        EnvironmentVariablesUpdate env_update,
        // TODO: use something like pipes instead of path to files
        const std::optional<std::string>& stdout_file = std::nullopt,
        const std::optional<std::string>& stderr_file = std::nullopt
    );

private:
    ev::ThreadControl& thread_control_;
};

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
