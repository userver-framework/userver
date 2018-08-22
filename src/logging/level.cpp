#include <yandex/taxi/userver/logging/level.hpp>

#include <stdexcept>
#include <unordered_map>

#include <boost/algorithm/string/join.hpp>
#include <boost/range/adaptor/map.hpp>

namespace logging {

Level LevelFromString(const std::string& level_name) {
  static const std::unordered_map<std::string, Level> kLevelMap = {
      {"trace", Level::kTrace}, {"debug", Level::kDebug},
      {"info", Level::kInfo},   {"warning", Level::kWarning},
      {"error", Level::kError}, {"critical", Level::kCritical},
  };

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
