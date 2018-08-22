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

inline spdlog::level::level_enum ToSpdlogLevel(Level level) {
  return static_cast<spdlog::level::level_enum>(level);
}

Level LevelFromString(const std::string&);

}  // namespace logging
