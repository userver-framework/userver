#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/logging/impl/logger_base.hpp>
#include <userver/utils/trivial_map.hpp>

namespace std {

template <>
struct hash<USERVER_NAMESPACE::logging::Level> {
  std::size_t operator()(USERVER_NAMESPACE::logging::Level level) const {
    return static_cast<std::size_t>(level);
  }
};

}  // namespace std

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

constexpr utils::TrivialBiMap kLevelMap = [](auto selector) {
  return selector()
      .Case("trace", Level::kTrace)
      .Case("debug", Level::kDebug)
      .Case("info", Level::kInfo)
      .Case("warning", Level::kWarning)
      .Case("error", Level::kError)
      .Case("critical", Level::kCritical)
      .Case("none", Level::kNone);
};

}  // namespace

Level LevelFromString(std::string_view level_name) {
  auto value = kLevelMap.TryFindICase(level_name);
  if (!value) {
    throw std::runtime_error(
        fmt::format("Unknown log level '{}' (must be one of {})", level_name,
                    kLevelMap.DescribeFirst()));
  }
  return *value;
}

std::string ToString(Level level) {
  auto value = kLevelMap.TryFind(level);
  if (!value) {
    return "Unknown (" + std::to_string(static_cast<int>(level)) + ')';
  }
  return std::string{*value};
}

std::optional<Level> OptionalLevelFromString(
    const std::optional<std::string>& level_name) {
  if (level_name)
    return LevelFromString(*level_name);
  else
    return std::nullopt;
}

}  // namespace logging

USERVER_NAMESPACE_END
