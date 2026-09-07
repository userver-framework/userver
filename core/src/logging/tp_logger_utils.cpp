#include <logging/tp_logger_utils.hpp>

#include <algorithm>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

#include <boost/filesystem/operations.hpp>

#include <logging/impl/fd_sink.hpp>
#include <logging/impl/file_sink.hpp>
#include <logging/impl/synchronized_sink.hpp>
#include <logging/impl/tcp_socket_sink.hpp>
#include <logging/impl/unix_socket_sink.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/log.hpp>
#include <userver/net/blocking/get_addr_info.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/map_to_array.hpp>

#include <unistd.h>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

namespace {

constexpr std::string_view kUnixSocketPrefix = "unix:";
constexpr std::string_view kDefaultLoggerName = "default";
constexpr std::string_view kNullPath = "@null";

void LogAndThrow(std::string msg) {
    LOG_ERROR() << msg;
    throw std::runtime_error(std::move(msg));
}

void CreateLogDirectory(const std::string& logger_name, const std::string& file_path) {
    try {
        const auto dirname = boost::filesystem::path(file_path).parent_path();
        boost::filesystem::create_directories(dirname);
    } catch (const std::exception& e) {
        LogAndThrow("Failed to create directory for log file of logger '" + logger_name + "': " + e.what());
    }
}

void RemoveOldFile(const std::string& logger_name, const std::string& file_path) {
    try {
        const auto path = boost::filesystem::path(file_path);
        boost::filesystem::remove(path);
    } catch (const std::exception& e) {
        LogAndThrow("Failed to remove old log file of logger '" + logger_name + "': " + e.what());
    }
}

SinkPtr GetSinkFromFilename(const std::string& file_path) {
    if (file_path.starts_with(kUnixSocketPrefix)) {
        // Use Unix-socket sink
        return std::make_unique<UnixSocketSink>(file_path.substr(kUnixSocketPrefix.size()));
    } else {
        return std::make_unique<FileSink>(file_path);
    }
}

SinkPtr MakeOptionalSink(const LoggerConfig& config, std::shared_ptr<std::mutex> write_mutex) {
    if (config.file_path == kNullPath) {
        return nullptr;
    }

    if (config.file_path == "@stderr") {
        if (write_mutex) {
            return MakeSynchronizedSink<UnownedFdSink>(std::move(write_mutex), STDERR_FILENO);
        }
        return std::make_unique<UnownedFdSink>(STDERR_FILENO);
    }
    if (config.file_path == "@stdout") {
        if (write_mutex) {
            return MakeSynchronizedSink<UnownedFdSink>(std::move(write_mutex), STDOUT_FILENO);
        }
        return std::make_unique<UnownedFdSink>(STDOUT_FILENO);
    }

    CreateLogDirectory(config.logger_name, config.file_path);
    if (config.truncate_on_start) {
        if (config.file_path.starts_with(kUnixSocketPrefix)) {
            LogAndThrow(
                "truncate-on-start cannot be combined with unix socket path for logger '" + config.logger_name + "': "
            );
        }
        RemoveOldFile(config.logger_name, config.file_path);
    }

    if (config.file_path.starts_with(kUnixSocketPrefix)) {
        return GetSinkFromFilename(config.file_path);
    }
    if (write_mutex) {
        return MakeSynchronizedSink<FileSink>(std::move(write_mutex), config.file_path);
    }
    return std::make_unique<FileSink>(config.file_path);
}

auto MakeTestsuiteSink(const TestsuiteCaptureConfig& config) {
    auto addrs = net::blocking::GetAddrInfo(config.host, std::to_string(config.port).c_str());
    return std::make_unique<TcpSocketSink>(std::move(addrs));
}

}  // namespace

SharedPathMutexes MakeSharedPathMutexes(const std::vector<LoggerConfig>& logger_configs) {
    std::unordered_map<std::string, std::size_t> path_counts;
    std::string default_logger_path;
    for (const auto& logger_config : logger_configs) {
        if (logger_config.logger_name == kDefaultLoggerName && default_logger_path.empty()) {
            default_logger_path = logger_config.file_path;
        }
        if (logger_config.file_path != kNullPath) {
            ++path_counts[logger_config.file_path];
        }
    }

    SharedPathMutexes result;
    for (const auto& [path, count] : path_counts) {
        if (count <= 1) {
            continue;
        }
        if (path == default_logger_path) {
            auto* const default_tp_logger = dynamic_cast<TpLogger*>(&logging::GetDefaultLogger());
            UINVARIANT(default_tp_logger, "default logger must be a TpLogger set up by components::Run");
            UINVARIANT(
                !default_tp_logger->GetSinks().empty(),
                "default logger for a shared log path must already have a primary sink"
            );
            const auto* synchronized_sink = dynamic_cast<
                const SynchronizedSinkBase*>(default_tp_logger->GetSinks().front().get());
            UINVARIANT(
                synchronized_sink,
                "default logger for a shared log path must use SynchronizedSink (set up in components::Run)"
            );
            result.emplace(path, synchronized_sink->GetMutex());
        } else {
            result.emplace(path, std::make_shared<std::mutex>());
        }
    }

    return result;
}

std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config) {
    return MakeTpLogger(config, std::shared_ptr<std::mutex>{});
}

std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config, std::shared_ptr<std::mutex> write_mutex) {
    auto logger = std::make_shared<TpLogger>(config.format, config.logger_name);
    logger->SetLevel(config.level);
    logger->SetFlushOn(config.flush_level);

    if (auto basic_sink = MakeOptionalSink(config, std::move(write_mutex))) {
        logger->AddSink(std::move(basic_sink));
    }

    if (config.testsuite_capture) {
        auto socket_sink_holder = MakeTestsuiteSink(*config.testsuite_capture);
        auto* const socket_sink = socket_sink_holder.get();
        logger->AddSink(std::move(socket_sink_holder));
        // Overwriting the level of TpLogger.
        socket_sink->SetLevel(logging::Level::kNone);
        // Deliver captured logs immediately: disable notification batching so every log notifies consumer.
        logger->SetNotificationBatching(false);
    }

    return logger;
}

std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config, const SharedPathMutexes& shared_path_mutexes) {
    std::shared_ptr<std::mutex> write_mutex;
    if (const auto mutex_it = shared_path_mutexes.find(config.file_path); mutex_it != shared_path_mutexes.end()) {
        write_mutex = mutex_it->second;
    }
    return MakeTpLogger(config, std::move(write_mutex));
}

std::shared_ptr<TpLogger> GetNonOwningDefaultLogger(const LoggerConfig& config) {
    UASSERT(config.logger_name == kDefaultLoggerName);

    auto* const default_tp_logger = dynamic_cast<TpLogger*>(&logging::GetDefaultLogger());
    UINVARIANT(
        default_tp_logger,
        "components::Run should set up the default logger using "
        "component_manager.components.logging.loggers.default section"
    );
    // Aliasing constructor, the resulting shared_ptr does not own the logger.
    return std::shared_ptr<TpLogger>(std::shared_ptr<TpLogger>(), default_tp_logger);
}

TcpSocketSink* GetTcpSocketSink(TpLogger& logger) {
    for (const auto& sink_ptr : logger.GetSinks()) {
        if (auto* const tcp_socket_sink = dynamic_cast<TcpSocketSink*>(sink_ptr.get())) {
            return tcp_socket_sink;
        }
    }
    return nullptr;
}

std::vector<LoggerConfig> ExtractLoggerConfigs(const components::ManagerConfig& config) {
    // NOLINTNEXTLINE(readability-qualified-auto)
    const auto logging_config = std::ranges::find_if(config.components, [](const auto& component_config) {
        return component_config.Name() == "logging";
    });
    if (logging_config == config.components.end()) {
        throw NoLoggerComponent(
            "No component config found for 'logging', which is a required "
            "component"
        );
    }

    if ((*logging_config)["loggers"].IsMissing()) {
        return {};
    }
    return yaml_config::ParseMapToArray<LoggerConfig>((*logging_config)["loggers"]);
}

}  // namespace logging::impl

USERVER_NAMESPACE_END
