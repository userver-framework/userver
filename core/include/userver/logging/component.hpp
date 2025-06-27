#pragma once

/// @file userver/logging/component.hpp
/// @brief @copybrief components::Logging

#include <string>
#include <unordered_map>

#include <userver/components/component_fwd.hpp>
#include <userver/components/raw_component_base.hpp>
#include <userver/concurrent/async_event_source.hpp>
#include <userver/os_signals/component.hpp>

#include <userver/rcu/rcu_map.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/periodic_task.hpp>
#include <userver/utils/statistics/entry.hpp>
#include <userver/utils/statistics/writer.hpp>

#include "logger.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging::impl {
class TpLogger;
class TcpSocketSink;
}  // namespace logging::impl

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
/// `fs-task-processor` | task processor for disk I/O operations | engine::current_task::GetBlockingTaskProcessor()
/// `loggers.<logger-name>.file_path` | path to the log file | -
/// `loggers.<logger-name>.level` | log verbosity | `info`
/// `loggers.<logger-name>.format` | log output format, one of `tskv`, `ltsv`, `json`, `json_yadeploy` | `tskv`
/// `loggers.<logger-name>.flush_level` | messages of this and higher levels get flushed to the file immediately | `warning`
/// `loggers.<logger-name>.message_queue_size` | the size of internal message queue, must be a power of 2 | 65536
/// `loggers.<logger-name>.overflow_behavior` | message handling policy while the queue is full: `discard` drops messages, `block` waits until message gets into the queue | `discard`
/// `loggers.<logger-name>.testsuite-capture` | if exists, setups additional TCP log sink for testing purposes | {}
/// `loggers.<logger-name>.fs-task-processor` | task processor for disk I/O operations for this logger | top-level `fs-task-processor` option
///
/// `default` logger is the one used for `LOG_*`.
///
/// ### Logs output
/// You can specify where logs are written in the `file_path` option:
/// - Use <tt>file_path: '@stdout'</tt> to write your logs to standard output stream;
/// - Use <tt>file_path: '@stderr'</tt> to write your logs to standard error stream;
/// - Use <tt>file_path: '@null'</tt> to suppress sending of logs;
/// - Use <tt>file_path: /absolute/path/to/log/file.log</tt> to write your logs to file. Use USR1 signal or @ref server::handlers::OnLogRotate to reopen files after log rotation;
/// - Use <tt>file_path: 'unix:/absolute/path/to/logs.sock'</tt> to write your logs to unix socket. Socket must be created before the service starts and closed by listener after service is shut down.
///
/// For sending logs directly to an otlp (opentelemetry) sink, see @ref opentelemetry.
///
/// Customization of log sinks beyond the ones listed above is not supported at the moment.
///
/// ### Logs format
/// You can specify in what format logs are written in the `format` option:
/// - Use `format: tskv` for traditional optimized userver-flavoured TSKV representation. See @ref utils::encoding::TskvParser for the exact format specification;
/// - Use `format: ltsv` for the same format, with `:` instead of `=` for key-value separator;
/// - Use `format: raw` for TSKV logs with timestamp and other default tags stripped, useful for @ref custom_loggers "custom loggers";
/// - Use `format: json` for JSON logs. @ref logging::JsonString can be used with this format for rich hierarchical log tags;
/// - Use `format: json_yadeploy` for JSON logs with a slightly different structure.
///
/// When sending logs using @ref opentelemetry, logs are written to otlp protobuf messages.
///
/// Customization of log formats beyond the ones listed above is not supported at the moment.
///
/// ### testsuite-capture options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// `loggers.<logger-name>.testsuite-capture.host` | testsuite hostname, e.g. `localhost` | -
/// `loggers.<logger-name>.testsuite-capture.host` | testsuite port | -
///
/// ## Static configuration examples:
///
/// Writing logs to stderr:
///
/// @snippet core/functional_tests/basic_chaos/static_config.yaml logging
///
/// Advanced configuration showing options for access logs and a custom opentracing logger:
///
/// @snippet components/common_component_list_test.cpp Sample logging component config

// clang-format on

class Logging final : public RawComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of components::Logging component
    static constexpr std::string_view kName = "logging";

    /// The component constructor
    Logging(const ComponentConfig&, const ComponentContext&);
    ~Logging() override;

    /// @brief Returns a logger by its name
    /// @param name Name of the logger
    /// @returns Pointer to the Logger instance
    /// @throws std::runtime_error if logger with this name is not registered
    logging::LoggerPtr GetLogger(const std::string& name);

    /// @brief Returns a text logger by its name
    /// @param name Name of the logger
    /// @returns Pointer to the Logger instance
    /// @throws std::runtime_error if logger with this name is not registered
    /// @throws std::runtime_error if logger is not a text logger
    logging::TextLoggerPtr GetTextLogger(const std::string& name);

    /// @brief Sets a logger
    /// @param name Name of the logger
    /// @param logger Logger to set
    void SetLogger(const std::string& name, logging::LoggerPtr logger);

    /// @brief Returns a logger by its name
    /// @param name Name of the logger
    /// @returns Pointer to the Logger instance, or `nullptr` if not registered
    logging::LoggerPtr GetLoggerOptional(const std::string& name);

    void StartSocketLoggingDebug(const std::optional<logging::Level>& log_level);
    void StopSocketLoggingDebug(const std::optional<logging::Level>& log_level);

    /// Reopens log files after rotation
    void OnLogRotate();
    void TryReopenFiles();

    void WriteStatistics(utils::statistics::Writer& writer) const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void Init(const ComponentConfig&, const ComponentContext&);
    void Stop() noexcept;

    void FlushLogs();

    engine::TaskProcessor& fs_task_processor_;
    std::unordered_map<std::string, std::shared_ptr<logging::impl::TpLogger>> loggers_;
    rcu::RcuMap<std::string, logging::LoggerPtr> extra_loggers_;
    utils::PeriodicTask flush_task_;
    logging::impl::TcpSocketSink* socket_sink_{nullptr};
    utils::statistics::MetricsStoragePtr metrics_storage_;

    // Subscriptions must be the last fields.
    os_signals::Subscriber signal_subscriber_;
    utils::statistics::Entry statistics_holder_;
};

template <>
inline constexpr bool kHasValidate<Logging> = true;

}  // namespace components

USERVER_NAMESPACE_END
