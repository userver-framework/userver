#include <userver/logging/component.hpp>

#include <chrono>
#include <stdexcept>

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>

#include <fmt/format.h>

#include <logging/impl/buffered_file_sink.hpp>
#include <logging/impl/fd_sink.hpp>
#include <logging/impl/reopening_file_sink.hpp>
#include <logging/impl/tcp_socket_sink.hpp>
#include <logging/impl/unix_socket_sink.hpp>
#include <logging/spdlog_helpers.hpp>
#include <logging/tp_logger.hpp>
#include <userver/components/component.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>
#include <userver/os_signals/component.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/thread_name.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/net/blocking/get_addr_info.hpp>
#include "config.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

constexpr std::chrono::seconds kDefaultFlushInterval{2};
constexpr std::string_view unix_socket_prefix = "unix:";

struct TestsuiteCaptureConfig {
  std::string host;
  int port{};
};

TestsuiteCaptureConfig Parse(const yaml_config::YamlConfig& value,
                             formats::parse::To<TestsuiteCaptureConfig>) {
  TestsuiteCaptureConfig config;
  config.host = value["host"].As<std::string>();
  config.port = value["port"].As<int>();
  return config;
}

std::optional<TestsuiteCaptureConfig> GetTestsuiteCaptureConfig(
    const yaml_config::YamlConfig& logger_config) {
  const auto& config = logger_config["testsuite-capture"];
  if (config.IsMissing()) return std::nullopt;
  return config.As<TestsuiteCaptureConfig>();
}

void ReopenAll(const std::shared_ptr<logging::impl::TpLogger>& logger) {
  int index = -1;
  for (const auto& s : logger->GetSinks()) {
    ++index;
    if (auto reop = std::dynamic_pointer_cast<logging::impl::BaseSink>(s);
        reop) {
      try {
        reop->Reopen(logging::impl::ReopenMode::kAppend);
      } catch (const std::exception& e) {
        LOG_ERROR() << "Exception on log reopen: " << e;
      }
    } else if (auto reop = std::dynamic_pointer_cast<
                   logging::impl::ReopeningFileSinkMT>(s);
               reop) {
      // TODO: Remove with condition after close experiments with new sink
      try {
        bool should_truncate = false;
        reop->Reopen(should_truncate);
      } catch (const std::exception& e) {
        LOG_ERROR() << "Exception on log reopen: " << e;
      }
    } else {
      LOG_INFO() << "Skipping rotation for sink #" << index << " from '"
                 << logger->GetLoggerName() << "' logger";
    }
  }
}

void CreateLogDirectory(const std::string& logger_name,
                        const std::string& file_path) {
  try {
    const auto dirname = boost::filesystem::path(file_path).parent_path();
    boost::filesystem::create_directories(dirname);
  } catch (const std::exception& e) {
    auto msg = "Failed to create directory for log file of logger '" +
               logger_name + "': " + e.what();
    LOG_ERROR() << msg;
    throw std::runtime_error(msg);
  }
}

spdlog::sink_ptr GetSinkFromFilename(const spdlog::filename_t& file_path) {
  if (boost::starts_with(file_path, unix_socket_prefix)) {
    // Use Unix-socket sink
    return std::make_shared<logging::impl::UnixSocketSink>(
        file_path.substr(unix_socket_prefix.size()));
  } else {
    // Use File sink
    // TODO: Set On experiment in prod
    if (logging::impl::kUseUserverSinks.IsEnabled()) {
      return std::make_shared<logging::impl::BufferedFileSink>(file_path);
    } else {
      return std::make_shared<logging::impl::ReopeningFileSinkMT>(file_path);
    }
  }
}

std::shared_ptr<logging::impl::TpLogger> CreateAsyncLogger(
    const std::string& logger_name, const logging::LoggerConfig& config) {
  auto logger =
      std::make_shared<logging::impl::TpLogger>(config.format, logger_name);
  spdlog::sink_ptr sink;
  if (config.file_path == "@null") {
    // do nothing
  } else if (config.file_path == "@stderr") {
    sink = std::make_shared<logging::impl::BufferedStderrFileSink>();
  } else if (config.file_path == "@stdout") {
    sink = std::make_shared<logging::impl::BufferedStdoutFileSink>();
  } else {
    CreateLogDirectory(logger_name, config.file_path);
    sink = GetSinkFromFilename(config.file_path);
  }

  if (sink) {
    logger->AddSink(std::move(sink));
  }

  logger->SetLevel(config.level);
  logger->SetPattern(config.pattern);
  logger->SetFlushOn(config.flush_level);

  return logger;
}

}  // namespace

namespace impl {

template <class Sink>
void AddSocketSink(const TestsuiteCaptureConfig& config, Sink& socket_sink,
                   logging::impl::TpLogger& logger) {
  auto addrs = net::blocking::GetAddrInfo(config.host,
                                          std::to_string(config.port).c_str());

  socket_sink =
      std::make_shared<logging::impl::TcpSocketSink>(std::move(addrs));

  logger.AddSink(socket_sink);

  // AddSink applies the logger level and patterns to the sink. Overwriting
  /// those.
  socket_sink->set_pattern(logging::GetSpdlogPattern(logging::Format::kTskv));
  socket_sink->set_level(spdlog::level::off);
}

}  // namespace impl

/// [Signals sample - init]
Logging::Logging(const ComponentConfig& config, const ComponentContext& context)
    : signal_subscriber_(
          context.FindComponent<os_signals::ProcessorComponent>()
              .Get()
              .AddListener(this, kName, SIGUSR1, &Logging::OnLogRotate))
/// [Signals sample - init]
{
  try {
    Init(config, context);
  } catch (const std::exception&) {
    Stop();
    throw;
  }
}

void Logging::Init(const ComponentConfig& config,
                   const ComponentContext& context) {
  const auto fs_task_processor_name =
      config["fs-task-processor"].As<std::string>();
  fs_task_processor_ = &context.GetTaskProcessor(fs_task_processor_name);

  const auto loggers = config["loggers"];

  for (const auto& [logger_name, logger_yaml] : Items(loggers)) {
    const bool is_default_logger = logger_name == "default";

    const auto logger_config = logger_yaml.As<logging::LoggerConfig>();
    const auto tp_name =
        logger_config.fs_task_processor.value_or(fs_task_processor_name);
    auto logger = CreateAsyncLogger(logger_name, logger_config);

    if (is_default_logger) {
      if (logger_config.queue_overflow_behavior ==
          logging::LoggerConfig::QueueOverflowBehavior::kBlock) {
        throw std::runtime_error(
            "'default' logger should not be set to 'overflow_behavior: block'! "
            "Default loggerr is used by the userver internals, including the "
            "logging internals. Blocking inside the engine internals could "
            "lead "
            "to hardly reproducible hangups in some border cases of error "
            "reporting.");
      }

      if (const auto& testsuite_config =
              GetTestsuiteCaptureConfig(logger_yaml)) {
        impl::AddSocketSink(*testsuite_config, socket_sink_, *logger);
      }

      logging::LogFlush();
      logging::impl::SetDefaultLoggerRef(*logger);

      // the default logger should outlive the component
      static logging::LoggerPtr default_component_logger_holder{};
      default_component_logger_holder = logger;
    }

    logger->StartAsync(context.GetTaskProcessor(tp_name),
                       logger_config.message_queue_size,
                       logger_config.queue_overflow_behavior);

    auto insertion_result = loggers_.emplace(logger_name, std::move(logger));
    if (!insertion_result.second) {
      throw std::runtime_error("duplicate logger '" +
                               insertion_result.first->first + '\'');
    }
  }
  flush_task_.Start("log_flusher",
                    utils::PeriodicTask::Settings(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            kDefaultFlushInterval),
                        {}, logging::Level::kTrace),
                    GetTaskFunction());
}

Logging::~Logging() { Stop(); }

void Logging::Stop() noexcept {
  /// [Signals sample - destr]
  signal_subscriber_.Unsubscribe();
  /// [Signals sample - destr]
  flush_task_.Stop();

  // Loggers could be used from non coroutine environments and should be
  // available even after task processors are down.
  for (const auto& [logger_name, logger] : loggers_) {
    logger->SwitchToSyncMode();
  }
}

logging::LoggerPtr Logging::GetLogger(const std::string& name) {
  auto it = loggers_.find(name);
  if (it == loggers_.end()) {
    throw std::runtime_error("logger '" + name + "' not found");
  }
  return it->second;
}

logging::LoggerPtr Logging::GetLoggerOptional(const std::string& name) {
  return utils::FindOrDefault(loggers_, name, nullptr);
}

void Logging::StartSocketLoggingDebug() {
  UASSERT(socket_sink_);
  socket_sink_->set_level(spdlog::level::trace);
}

void Logging::StopSocketLoggingDebug() {
  UASSERT(socket_sink_);
  logging::LogFlush();
  socket_sink_->set_level(spdlog::level::off);
  socket_sink_->Close();
}

void Logging::OnLogRotate() {
  try {
    TryReopenFiles();

  } catch (const std::exception& e) {
    LOG_ERROR() << "An error occurred while ReopenAll: " << e;
  }
}

void Logging::TryReopenFiles() {
  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.reserve(loggers_.size() + 1);
  for (const auto& item : loggers_) {
    tasks.push_back(engine::CriticalAsyncNoSpan(*fs_task_processor_, ReopenAll,
                                                item.second));
  }

  std::string result_messages;

  for (auto& task : tasks) {
    try {
      task.Get();
    } catch (const std::exception& e) {
      result_messages += e.what();
      result_messages += ";";
    }
  }
  LOG_INFO() << "Log rotated";

  if (!result_messages.empty()) {
    throw std::runtime_error("ReopenAll errors: " + result_messages);
  }
}

void Logging::FlushLogs() {
  logging::LogFlush();
  for (auto& item : loggers_) {
    item.second->Flush();
  }
}

yaml_config::Schema Logging::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<impl::ComponentBase>(R"(
type: object
description: Logging component
additionalProperties: false
properties:
    fs-task-processor:
        type: string
        description: task processor for disk I/O operations
    loggers:
        type: object
        description: logger options
        properties: {}
        additionalProperties:
            type: object
            description: logger options
            additionalProperties: false
            properties:
                file_path:
                    type: string
                    description: path to the log file
                level:
                    type: string
                    description: log verbosity
                    defaultDescription: info
                format:
                    type: string
                    description: log output format
                    defaultDescription: tskv
                    enum:
                      - tskv
                      - ltsv
                      - raw
                flush_level:
                    type: string
                    description: messages of this and higher levels get flushed to the file immediately
                    defaultDescription: warning
                message_queue_size:
                    type: integer
                    description: the size of internal message queue, must be a power of 2
                    defaultDescription: 65536
                overflow_behavior:
                    type: string
                    description: "message handling policy while the queue is full: `discard` drops messages, `block` waits until message gets into the queue"
                    defaultDescription: discard
                    enum:
                      - discard
                      - block
                fs-task-processor:
                    type: string
                    description: task processor for disk I/O operations for this logger
                    defaultDescription: fs-task-processor of the loggers component
                testsuite-capture:
                    type: object
                    description: if exists, setups additional TCP log sink for testing purposes
                    defaultDescription: "{}"
                    additionalProperties: false
                    properties:
                        host:
                            type: string
                            description: testsuite hostname, e.g. localhost
                        port:
                            type: integer
                            description: testsuite port
)");
}

}  // namespace components

USERVER_NAMESPACE_END
