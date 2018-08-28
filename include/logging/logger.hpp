#pragma once

/// @file logging/logger.hpp
/// @brief Logger definitions and helpers

#include <string>

#include <spdlog/spdlog.h>

namespace logging {

using Logger = spdlog::logger;
using LoggerPtr = std::shared_ptr<Logger>;

/// @brief Creates synchronous stderr logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeStderrLogger(const std::string& name);

/// @brief Creates synchronous file logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @param path target log file path
/// @see components::Logging
LoggerPtr MakeFileLogger(const std::string& name, const std::string& path);

}  // namespace logging
