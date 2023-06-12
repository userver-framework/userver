#pragma once

/// @file userver/logging/logger.hpp
/// @brief Logger definitions and helpers

#include <memory>
#include <string>

#include <userver/logging/format.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/null_logger.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

class LoggerBase;
void LogRaw(LoggerBase& logger, Level level, std::string_view message);

}  // namespace impl

using LoggerPtr = std::shared_ptr<impl::LoggerBase>;

/// @brief Creates synchronous stderr logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeStderrLogger(const std::string& name, Format format,
                           Level level = Level::kInfo);

/// @brief Creates synchronous stdout logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeStdoutLogger(const std::string& name, Format format,
                           Level level = Level::kInfo);

/// @brief Creates synchronous file logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @param path target log file path
/// @see components::Logging
LoggerPtr MakeFileLogger(const std::string& name, const std::string& path,
                         Format format, Level level = Level::kInfo);

}  // namespace logging

USERVER_NAMESPACE_END
