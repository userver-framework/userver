#pragma once

#include <string>

#include <spdlog/spdlog.h>

namespace logging {

using Logger = spdlog::logger;
using LoggerPtr = std::shared_ptr<Logger>;

// Create synchronous loggers with default pattern
LoggerPtr MakeStderrLogger(const std::string& name);
LoggerPtr MakeFileLogger(const std::string& name, const std::string& path);

}  // namespace logging
