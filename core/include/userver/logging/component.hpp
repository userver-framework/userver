#pragma once

/// @file logging/component.hpp
/// @brief @copybrief components::Logging

#include <string>
#include <unordered_map>

#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <components/impl/component_base.hpp>

#include <utils/periodic_task.hpp>

#include "logger.hpp"

namespace logging {
struct LoggerConfig;
}

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief %Logging component
///
/// Allows to configure the default logger and/or additional loggers for your
/// needs.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// file_path | (*required*) path to the log file | --
/// level | log verbosity | info
/// pattern | message formatting pattern, see [spdlog wiki](https://github.com/gabime/spdlog/wiki/3.-Custom-formatting#pattern-flags) for details, %%v means message text | tskv prologue with timestamp, timezone and level fields
/// flush_level | messages of this and higher levels get flushed to the file immediately | warning
/// message_queue_size | the size of internal message queue, must be a power of 2 | 65536
/// overflow_behavior | message handling policy while the queue is full: `discard` drops messages, `block` waits until message gets into the queue | discard
///
/// ## Static configuration example:
///
/// @snippet components/common_component_list_test.cpp Sample logging component config
///
/// `default` section configures the default logger for LOG_*.

// clang-format on

class Logging final : public impl::ComponentBase {
 public:
  /// The default name of this component
  static constexpr const char* kName = "logging";

  /// The component constructor
  Logging(const ComponentConfig&, const ComponentContext&);
  ~Logging() override;

  /// @brief Returns a logger by its name
  /// @param name Name of the logger
  /// @returns Pointer to the Logger instance
  /// @throws std::runtime_error if logger with this name is not registered
  logging::LoggerPtr GetLogger(const std::string& name);

  /// Reopens log files after rotation
  void OnLogRotate();

 private:
  auto GetTaskFunction() {
    return [this] { FlushLogs(); };
  }
  void FlushLogs();

  std::shared_ptr<spdlog::logger> CreateLogger(
      const std::string& logger_name,
      const logging::LoggerConfig& logger_config, bool is_default_logger);

  engine::TaskProcessor* fs_task_processor_;
  std::vector<logging::ThreadPoolPtr> thread_pools_;
  std::unordered_map<std::string, logging::LoggerPtr> loggers_;
  utils::PeriodicTask flush_task_;
};

}  // namespace components
