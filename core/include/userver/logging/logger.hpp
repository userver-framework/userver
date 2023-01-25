#pragma once

/// @file userver/logging/logger.hpp
/// @brief Logger definitions and helpers

#include <memory>
#include <string>

#include "format.hpp"
#include "level.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace impl {

class LoggerWithInfo;
void LogRaw(LoggerWithInfo& logger, Level level, std::string_view message);

}  // namespace impl

using LoggerPtr = std::shared_ptr<impl::LoggerWithInfo>;

/// @brief Creates synchronous stderr logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeStderrLogger(const std::string& name, Format format,
                           Level level = Level::kInfo);

// TODO: remove after userver up
inline LoggerPtr MakeStderrLogger(const std::string& name,
                                  Level level = Level::kInfo) {
  return logging::MakeStderrLogger(name, Format::kTskv, level);
}

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

/// @brief Creates a logger that drops all incoming messages
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeNullLogger(const std::string& name);

}  // namespace logging

USERVER_NAMESPACE_END
