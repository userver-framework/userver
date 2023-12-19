#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>

#include <stdexcept>

#include <fmt/format.h>

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

constexpr utils::TrivialBiMap kLowerCaseLevelMap = [](auto selector) {
  return selector()
      .Case("trace", Level::kTrace)
      .Case("debug", Level::kDebug)
      .Case("info", Level::kInfo)
      .Case("warning", Level::kWarning)
      .Case("error", Level::kError)
      .Case("critical", Level::kCritical)
      .Case("none", Level::kNone);
};

constexpr utils::TrivialBiMap kUpperCaseLevelMap = [](auto selector) {
  return selector()
      .Case("TRACE", Level::kTrace)
      .Case("DEBUG", Level::kDebug)
      .Case("INFO", Level::kInfo)
      .Case("WARNING", Level::kWarning)
      .Case("ERROR", Level::kError)
      .Case("CRITICAL", Level::kCritical)
      .Case("NONE", Level::kNone);
};

}  // namespace

Level LevelFromString(std::string_view level_name) {
  auto value = kLowerCaseLevelMap.TryFindICase(level_name);
  if (!value) {
    throw std::runtime_error(
        fmt::format("Unknown log level '{}' (must be one of {})", level_name,
                    kLowerCaseLevelMap.DescribeFirst()));
  }
  return *value;
}

std::string_view ToString(Level level) {
  return utils::impl::EnumToStringView(level, kLowerCaseLevelMap);
}

std::string_view ToUpperCaseString(Level level) noexcept {
  const auto value = kUpperCaseLevelMap.TryFind(level);
  if (!value) {
    UASSERT_MSG(false, "Invalid value of Level enum");
    // Throwing here would lead to losing logs in production.
    return "ERROR";
  }
  return *value;
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
