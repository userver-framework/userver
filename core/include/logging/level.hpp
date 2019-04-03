#pragma once

/// @file logging/level.hpp
/// @brief Log levels

#include <array>
#include <atomic>
#include <string>

#include <boost/optional.hpp>

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

boost::optional<Level> OptionalLevelFromString(
    const boost::optional<std::string>& level_name);

inline auto& GetShouldLogCache() noexcept {
  static std::array<std::atomic<bool>, kLevelMax + 1> values{
      false, false, true, true, true, true, false};
  return values;
}

bool ShouldLog(Level level) noexcept;

}  // namespace logging
