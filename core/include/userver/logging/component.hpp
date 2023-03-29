#pragma once

/// @file userver/logging/component.hpp
/// @brief @copybrief components::Logging

#include <string>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/components/impl/component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/os_signals/component.hpp>

#include <userver/utils/periodic_task.hpp>

#include "logger.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {
struct LoggerConfig;

namespace impl {
class TpLogger;
class TcpSocketSink;
}  // namespace impl

}  // namespace logging

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
/// file_path | path to the log file | -
/// level | log verbosity | info
/// format | log output format, either `tskv` or `ltsv` | tskv
/// flush_level | messages of this and higher levels get flushed to the file immediately | warning
/// message_queue_size | the size of internal message queue, must be a power of 2 | 65536
/// overflow_behavior | message handling policy while the queue is full: `discard` drops messages, `block` waits until message gets into the queue | discard
/// testsuite-capture | if exists, setups additional TCP log sink for testing purposes | {}
/// fs-task-processor | task processor for disk I/O operations for this logger | fs-task-processor of the loggers component
///
/// ### Logs output
/// You can specify logger output, in `file_path` option:
/// - Use `@stdout` to write your logs to standard output stream;
/// - Use `@stderr` to write your logs to standard error stream;
/// - Use `@null` to suppress sending of logs;
/// - Use `%file_name%` to write your logs in file. Use USR1 signal or `OnLogRotate` handler to reopen files after log rotation;
/// - Use `unix:%socket_name%` to write your logs to unix socket. Socket must be created before the service starts and closed by listener afert service is shuted down.
///
/// ### testsuite-capture options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// host | testsuite hostname, e.g. localhost | -
/// port | testsuite port | -
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
  static constexpr std::string_view kName = "logging";

  /// The component constructor
  Logging(const ComponentConfig&, const ComponentContext&);
  ~Logging() override;

  /// @brief Returns a logger by its name
  /// @param name Name of the logger
  /// @returns Pointer to the Logger instance
  /// @throws std::runtime_error if logger with this name is not registered
  logging::LoggerPtr GetLogger(const std::string& name);

  /// @brief Returns a logger by its name
  /// @param name Name of the logger
  /// @returns Pointer to the Logger instance, or `nullptr` if not registered
  logging::LoggerPtr GetLoggerOptional(const std::string& name);

  void StartSocketLoggingDebug();
  void StopSocketLoggingDebug();

  /// Reopens log files after rotation
  void OnLogRotate();
  void TryReopenFiles();

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  void Init(const ComponentConfig&, const ComponentContext&);
  void Stop() noexcept;

  auto GetTaskFunction() {
    return [this] { FlushLogs(); };
  }
  void FlushLogs();

  engine::TaskProcessor* fs_task_processor_{nullptr};
  std::unordered_map<std::string, std::shared_ptr<logging::impl::TpLogger>>
      loggers_;
  utils::PeriodicTask flush_task_;
  std::shared_ptr<logging::impl::TcpSocketSink> socket_sink_;
  os_signals::Subscriber signal_subscriber_;
};

template <>
inline constexpr bool kHasValidate<Logging> = true;

}  // namespace components

USERVER_NAMESPACE_END
