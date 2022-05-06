#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>

#include <functional>
#include <stdexcept>
#include <unordered_map>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>

#include <logging/dynamic_debug.hpp>
#include <logging/get_should_log_cache.hpp>
#include <logging/spdlog.hpp>
#include <userver/rcu/rcu.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/str_icase.hpp>

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

static_assert(static_cast<spdlog::level::level_enum>(Level::kTrace) ==
                  spdlog::level::trace,
              "kTrace enumerator value doesn't match spdlog's trace");
static_assert(static_cast<spdlog::level::level_enum>(Level::kDebug) ==
                  spdlog::level::debug,
              "kDebug enumerator value doesn't match spdlog's debug");
static_assert(static_cast<spdlog::level::level_enum>(Level::kInfo) ==
                  spdlog::level::info,
              "kInfo enumerator value doesn't match spdlog's info");
static_assert(static_cast<spdlog::level::level_enum>(Level::kWarning) ==
                  spdlog::level::warn,
              "kWarning enumerator value doesn't match spdlog's warn");
static_assert(static_cast<spdlog::level::level_enum>(Level::kError) ==
                  spdlog::level::err,
              "kError enumerator value doesn't match spdlog's err");
static_assert(static_cast<spdlog::level::level_enum>(Level::kCritical) ==
                  spdlog::level::critical,
              "kCritical enumerator value doesn't match spdlog's critical");
static_assert(static_cast<spdlog::level::level_enum>(Level::kNone) ==
                  spdlog::level::off,
              "kNone enumerator value doesn't match spdlog's off");

namespace {

std::unordered_map<std::string, Level, utils::StrIcaseHash,
                   utils::StrIcaseEqual>
InitLevelMap() {
  return {{"trace", Level::kTrace}, {"debug", Level::kDebug},
          {"info", Level::kInfo},   {"warning", Level::kWarning},
          {"error", Level::kError}, {"critical", Level::kCritical},
          {"none", Level::kNone}};
}

std::unordered_map<Level, std::string> InitLevelToStringMap() {
  auto level_map = InitLevelMap();
  std::unordered_map<Level, std::string> result;
  for (const auto& elem : level_map) {
    result.emplace(elem.second, elem.first);
  }
  return result;
}

}  // namespace

Level LevelFromString(const std::string& level_name) {
  static const auto kLevelMap = InitLevelMap();

  auto it = kLevelMap.find(level_name);
  if (it == kLevelMap.end()) {
    throw std::runtime_error(
        "Unknown log level '" + level_name + "' (must be one of '" +
        boost::algorithm::join(kLevelMap | boost::adaptors::map_keys, "', '") +
        "')");
  }
  return it->second;
}

std::string ToString(Level level) {
  static const auto kLevelToStringMap = InitLevelToStringMap();
  auto it = kLevelToStringMap.find(level);
  if (it == kLevelToStringMap.end()) {
    return "Unknown (" + std::to_string(static_cast<int>(level)) + ')';
  }
  return it->second;
}

std::optional<Level> OptionalLevelFromString(
    const std::optional<std::string>& level_name) {
  if (level_name)
    return LevelFromString(*level_name);
  else
    return std::nullopt;
}

bool ShouldLogNospan(Level level) noexcept {
  return GetShouldLogCache()[static_cast<size_t>(level)];
}

bool ShouldLog(Level level, std::string_view location) noexcept {
  if (DynamicDebugShouldLog(location)) return true;

  if (!ShouldLogNospan(level)) return false;

  auto* span = tracing::Span::CurrentSpanUnchecked();
  if (span) {
    auto local_log_level = span->GetLocalLogLevel();
    if (local_log_level && *local_log_level > level) {
      return false;
    }
  }

  return true;
}

bool ShouldLog(Level level) noexcept { return ShouldLog(level, {}); }

}  // namespace logging

USERVER_NAMESPACE_END
