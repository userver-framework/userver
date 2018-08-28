#pragma once

/// @file logging/level.hpp
/// @brief Log levels

#include <string>

#include <spdlog/common.h>

namespace logging {

/// Log levels
enum class Level {
  kTrace = spdlog::level::trace,
  kDebug = spdlog::level::debug,
  kInfo = spdlog::level::info,
  kWarning = spdlog::level::warn,
  kError = spdlog::level::err,
  kCritical = spdlog::level::critical
};

/// Converts lowercase level name to a corresponding Level
Level LevelFromString(const std::string&);

namespace impl {

inline spdlog::level::level_enum ToSpdlogLevel(Level level) {
  return static_cast<spdlog::level::level_enum>(level);
}

}  // namespace impl
}  // namespace logging
