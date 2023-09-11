#pragma once

#include <memory>
#include <optional>

#include <components/manager_config.hpp>
#include <logging/config.hpp>
#include <logging/tp_logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging::impl {

class TcpSocketSink;

std::shared_ptr<TpLogger> MakeTpLogger(const LoggerConfig& config);

std::shared_ptr<TpLogger> GetDefaultLoggerOrMakeTpLogger(
    const LoggerConfig& config);

TcpSocketSink* GetTcpSocketSink(TpLogger& logger);

LoggerConfig ExtractDefaultLoggerConfig(
    const components::ManagerConfig& config);

}  // namespace logging::impl

USERVER_NAMESPACE_END
