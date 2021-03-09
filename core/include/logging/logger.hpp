#pragma once

/// @file logging/logger.hpp
/// @brief Logger definitions and helpers

#include <memory>
#include <string>

#include "level.hpp"

// Forwards for spdlog
namespace spdlog {

class logger;

namespace details {

class thread_pool;

}  // namespace details

}  // namespace spdlog

namespace logging {

using Logger = spdlog::logger;
using LoggerPtr = std::shared_ptr<Logger>;

using ThreadPool = spdlog::details::thread_pool;
using ThreadPoolPtr = std::shared_ptr<ThreadPool>;

/// @brief Creates synchronous stderr logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeStderrLogger(const std::string& name, Level level = Level::kInfo);

/// @brief Creates synchronous stdout logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeStdoutLogger(const std::string& name, Level level = Level::kInfo);

/// @brief Creates synchronous file logger with default tskv pattern
/// @param name logger name, for internal use, must be unique
/// @param path target log file path
/// @see components::Logging
LoggerPtr MakeFileLogger(const std::string& name, const std::string& path,
                         Level level = Level::kInfo);

/// @brief Creates a logger that drops all incoming messages
/// @param name logger name, for internal use, must be unique
/// @see components::Logging
LoggerPtr MakeNullLogger(const std::string& name);

}  // namespace logging
