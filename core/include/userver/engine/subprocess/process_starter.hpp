#pragma once

/// @file userver/engine/subprocess/process_starter.hpp
/// @brief @copybrief engine::subprocess::ProcessStarter

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/engine/subprocess/child_process.hpp>
#include <userver/engine/subprocess/environment_variables.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
class ThreadControl;
}  // namespace engine::ev

namespace engine::subprocess {

/// @ingroup userver_clients
///
/// @brief Creates a new OS subprocess and executes a command in it.
class ProcessStarter {
 public:
  /// @param task_processor will be used for executing asynchronous fork + exec.
  /// `main-task-processor is OK for this purpose.
  explicit ProcessStarter(TaskProcessor& task_processor);

  /// @param command the absolute path to the executable
  /// @param args exact args passed to the executable
  /// @param env redefines all environment variables
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      const EnvironmentVariables& env,
      // TODO: use something like pipes instead of path to files
      const std::optional<std::string>& stdout_file = std::nullopt,
      const std::optional<std::string>& stderr_file = std::nullopt);

  /// @param command the absolute path to the executable
  /// @param args exact args passed to the executable
  /// @param env_update variables to add to the current environment, overwriting
  /// existing ones
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      EnvironmentVariablesUpdate env_update = EnvironmentVariablesUpdate{{}},
      // TODO: use something like pipes instead of path to files
      const std::optional<std::string>& stdout_file = std::nullopt,
      const std::optional<std::string>& stderr_file = std::nullopt);

 private:
  ev::ThreadControl& thread_control_;
};

}  // namespace engine::subprocess

USERVER_NAMESPACE_END
