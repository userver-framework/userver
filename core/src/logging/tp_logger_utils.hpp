#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <components/manager_config.hpp>
#include <logging/config.hpp>
#include <logging/tp_logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class TcpSocketSink;

using SharedPathMutexes = std::unordered_map<std::string, std::shared_ptr<std::mutex>>;

/// Creates one mutex per `file_path` that is used by more than one logger.
/// If the default logger was already wrapped in components::Run, reuses its mutex
/// so all writers lock the same instance.
SharedPathMutexes MakeSharedPathMutexes(const std::vector<LoggerConfig>& logger_configs);

std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config);

/// If non-null, the primary sink is wrapped in @ref SynchronizedSink.
std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config, std::shared_ptr<std::mutex> write_mutex);

/// Looks up a mutex for `config.file_path` in @p shared_path_mutexes (if any)
/// and wraps the primary sink in @ref SynchronizedSink when found.
std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config, const SharedPathMutexes& shared_path_mutexes);

std::shared_ptr<TpLogger> GetNonOwningDefaultLogger(const LoggerConfig& config);

TcpSocketSink* GetTcpSocketSink(TpLogger& logger);

class NoLoggerComponent final : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

std::vector<LoggerConfig> ExtractLoggerConfigs(const components::ManagerConfig& config);

}  // namespace logging::impl

USERVER_NAMESPACE_END
