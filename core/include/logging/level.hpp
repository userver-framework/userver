#pragma once

/// @file logging/level.hpp
/// @brief Log levels

#include <array>
#include <atomic>
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

static const auto kLevelMax = static_cast<int>(Level::kNone);

/// Converts lowercase level name to a corresponding Level
Level LevelFromString(const std::string&);

inline auto& GetShouldLogCache() {
  static std::array<std::atomic<bool>, kLevelMax + 1> values{
      false, false, true, true, true, true, false};
  return values;
}

inline bool ShouldLog(Level level) {
  return GetShouldLogCache()[static_cast<size_t>(level)];
}

}  // namespace logging
