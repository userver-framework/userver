#include <logging/tp_logger_utils.hpp>

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/filesystem/operations.hpp>
#include <boost/range/algorithm/find_if.hpp>

#include <logging/impl/buffered_file_sink.hpp>
#include <logging/impl/tcp_socket_sink.hpp>
#include <logging/impl/unix_socket_sink.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>
#include <userver/net/blocking/get_addr_info.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

constexpr std::string_view kUnixSocketPrefix = "unix:";
constexpr std::string_view kDefaultLoggerName = "default";

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

SinkPtr GetSinkFromFilename(const std::string& file_path) {
  if (utils::text::StartsWith(file_path, kUnixSocketPrefix)) {
    // Use Unix-socket sink
    return std::make_unique<UnixSocketSink>(
        file_path.substr(kUnixSocketPrefix.size()));
  } else {
    return std::make_unique<BufferedFileSink>(file_path);
  }
}

SinkPtr MakeOptionalSink(const LoggerConfig& config) {
  if (config.file_path == "@null") {
    return nullptr;
  } else if (config.file_path == "@stderr") {
    return std::make_unique<logging::impl::BufferedUnownedFileSink>(stderr);
  } else if (config.file_path == "@stdout") {
    return std::make_unique<logging::impl::BufferedUnownedFileSink>(stdout);
  } else {
    CreateLogDirectory(config.logger_name, config.file_path);
    return GetSinkFromFilename(config.file_path);
  }
}

auto MakeTestsuiteSink(const TestsuiteCaptureConfig& config) {
  auto addrs = net::blocking::GetAddrInfo(config.host,
                                          std::to_string(config.port).c_str());
  return std::make_unique<TcpSocketSink>(std::move(addrs));
}

}  // namespace

std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config) {
  auto logger = std::make_shared<TpLogger>(config.format, config.logger_name);
  logger->SetLevel(config.level);
  logger->SetFlushOn(config.flush_level);

  if (auto basic_sink = MakeOptionalSink(config)) {
    logger->AddSink(std::move(basic_sink));
  }

  if (config.testsuite_capture) {
    auto socket_sink_holder = MakeTestsuiteSink(*config.testsuite_capture);
    auto* const socket_sink = socket_sink_holder.get();
    logger->AddSink(std::move(socket_sink_holder));
    // Overwriting the level of TpLogger.
    socket_sink->SetLevel(logging::Level::kNone);
  }

  return logger;
}

std::shared_ptr<TpLogger> GetDefaultLoggerOrMakeTpLogger(
    const LoggerConfig& config) {
  if (config.logger_name == kDefaultLoggerName) {
    auto* const default_tp_logger =
        dynamic_cast<logging::impl::TpLogger*>(&logging::GetDefaultLogger());
    UINVARIANT(default_tp_logger,
               "components::Run should set up the default logger using "
               "component_manager.components.logging.loggers.default section");
    // Aliasing constructor, the resulting shared_ptr does not own the logger.
    return std::shared_ptr<TpLogger>(std::shared_ptr<TpLogger>(),
                                     default_tp_logger);
  }
  return MakeTpLogger(config);
}

TcpSocketSink* GetTcpSocketSink(TpLogger& logger) {
  for (const auto& sink_ptr : logger.GetSinks()) {
    if (auto* const tcp_socket_sink =
            dynamic_cast<TcpSocketSink*>(sink_ptr.get())) {
      return tcp_socket_sink;
    }
  }
  return nullptr;
}

LoggerConfig ExtractDefaultLoggerConfig(
    const components::ManagerConfig& config) {
  // Note: this is a slight violation of separation of concerns. The component
  // system itself should not specifically care about 'logging' component, but
  // in this case it does. This is required to seamlessly enable logs coming
  // from the engine, from the component system, and from the core components.

  // NOLINTNEXTLINE(readability-qualified-auto)
  const auto iter = boost::find_if(config.components, [](const auto& config) {
    return config.Name() == "logging";
  });
  if (iter == config.components.end()) {
    throw std::runtime_error(
        "No component config found for 'logging', which is a required "
        "component");
  }

  const auto logger_config_yaml = (*iter)["loggers"][kDefaultLoggerName];
  auto logger_config = logger_config_yaml.As<LoggerConfig>();

  logger_config.SetName(std::string{kDefaultLoggerName});
  return logger_config;
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
