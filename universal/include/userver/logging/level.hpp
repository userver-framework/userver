#pragma once

/// @file userver/logging/level.hpp
/// @brief Log levels

#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

/// Logging macro and utilities
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

inline constexpr auto kLevelMax = static_cast<int>(Level::kNone);

/// @brief Converts lowercase level name to a corresponding Level, throws
/// std::runtime_error if no matching log level found.
Level LevelFromString(std::string_view);

/// @brief Returns a string representation of logging level, e.g. "info"
std::string_view ToString(Level level);

/// @brief Returns a string representation of logging level, e.g. "INFO"
std::string_view ToUpperCaseString(Level level) noexcept;

/// @brief Returns std::nullopt if level_name is std::nullopt, otherwise
/// behaves exactly like logging::LevelFromString.
std::optional<Level> OptionalLevelFromString(
    const std::optional<std::string>& level_name);

}  // namespace logging

USERVER_NAMESPACE_END
