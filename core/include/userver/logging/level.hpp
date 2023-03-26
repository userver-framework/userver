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

/// @brief Returns a string representation of logging level
std::string ToString(Level level);

/// @brief Returns std::nullopt if level_name is std::nullopt, otherwise
/// behaves exactly like logging::LevelFromString.
std::optional<Level> OptionalLevelFromString(
    const std::optional<std::string>& level_name);

/// @brief Returns true if the provided log level is greater or equal to
/// the current log level. Note that this function does not deal with
/// tracing::Span local log level, so the log may be actually disabled by the
/// span.
bool ShouldLogNospan(Level level) noexcept;

/// @brief Returns true if the provided log level is greater or equal to
/// the current log level and to the tracing::Span local log level. In other
/// words, returns true if the log with `level` is logged.
bool ShouldLog(Level level) noexcept;

}  // namespace logging

USERVER_NAMESPACE_END
