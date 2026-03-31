#pragma once

/// @file userver/logging/level.hpp
/// @brief Log levels

#include <cstdint>
#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

/// Logging macro and utilities
namespace logging {

/// Log levels
enum class Level : std::uint8_t {
    kTrace = 0,     ///< Level for very verbose debug messages
    kDebug = 1,     ///< Level for debug messages
    kInfo = 2,      ///< Level for non-error informational messages
    kWarning = 3,   ///< Level for warning messages
    kError = 4,     ///< Level for error messages
    kCritical = 5,  ///< Level for fatal error messages that can not be disabled
    kNone = 6,      ///< "Do not output messages" level
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
std::optional<Level> OptionalLevelFromString(const std::optional<std::string>& level_name);

}  // namespace logging

USERVER_NAMESPACE_END
