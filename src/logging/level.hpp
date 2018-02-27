#pragma once

#include <string>

#include <spdlog/common.h>

namespace logging {

enum class Level {
  kTrace = spdlog::level::trace,
  kDebug = spdlog::level::debug,
  kInfo = spdlog::level::info,
  kWarning = spdlog::level::warn,
  kError = spdlog::level::err,
  kCritical = spdlog::level::critical
};

Level LevelFromString(const std::string&);

}  // namespace logging
