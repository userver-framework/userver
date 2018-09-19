#include <logging/level.hpp>
#include <logging/log.hpp>

#include <stdexcept>
#include <unordered_map>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>

#include <logging/log_config.hpp>

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

bool ShouldLog(Level level) {
  return DefaultLogger()->should_log(
      static_cast<spdlog::level::level_enum>(level));
}

Level LevelFromString(const std::string& level_name) {
  static const std::unordered_map<std::string, Level> kLevelMap = {
      {"trace", Level::kTrace}, {"debug", Level::kDebug},
      {"info", Level::kInfo},   {"warning", Level::kWarning},
      {"error", Level::kError}, {"critical", Level::kCritical},
      {"none", Level::kNone}};

  auto it = kLevelMap.find(level_name);
  if (it == kLevelMap.end()) {
    throw std::runtime_error(
        "Unknown log level '" + level_name + "' (must be one of '" +
        boost::algorithm::join(kLevelMap | boost::adaptors::map_keys, "', '") +
        "')");
  }
  return it->second;
}

}  // namespace logging
