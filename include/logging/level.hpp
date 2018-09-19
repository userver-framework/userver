#pragma once

/// @file logging/level.hpp
/// @brief Log levels

#include <string>

namespace logging {

/// Log levels
enum class Level {
  kTrace = 0,
  kDebug = 1,
  kInfo = 2,
  kWarning = 3,
  kError = 4,
  kCritical = 5,
  kNone = 6
};

/// Converts lowercase level name to a corresponding Level
Level LevelFromString(const std::string&);

bool ShouldLog(Level level);

}  // namespace logging
