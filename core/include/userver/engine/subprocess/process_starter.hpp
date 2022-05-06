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

namespace engine {

namespace ev {
class ThreadControl;
}  // namespace ev

namespace subprocess {

/// @ingroup userver_clients
///
/// @brief Creates a new OS subprocess and executes a command in it.
class ProcessStarter {
 public:
  explicit ProcessStarter(TaskProcessor& task_processor);

  /// `env` redefines all environment variables.
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      const EnvironmentVariables& env,
      // TODO: use something like pipes instead of path to files
      const std::optional<std::string>& stdout_file = std::nullopt,
      const std::optional<std::string>& stderr_file = std::nullopt);

  /// Variables from `env_update` will be added to current environment.
  /// Existing values will be replaced.
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      EnvironmentVariablesUpdate env_update,
      // TODO: use something like pipes instead of path to files
      const std::optional<std::string>& stdout_file = std::nullopt,
      const std::optional<std::string>& stderr_file = std::nullopt);

  /// Exec subprocess using current environment.
  ChildProcess Exec(
      const std::string& command, const std::vector<std::string>& args,
      // TODO: use something like pipes instead of path to files
      const std::optional<std::string>& stdout_file = std::nullopt,
      const std::optional<std::string>& stderr_file = std::nullopt);

 private:
  ev::ThreadControl& thread_control_;
};

}  // namespace subprocess
}  // namespace engine

USERVER_NAMESPACE_END
