#include "redis_info.hpp"

#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

ReplicationInfo ParseReplicationInfo(const std::string& info) {
  ReplicationInfo ret;

  const auto lines = utils::text::Split(info, "\n\r");
  for (const auto& line : lines) {
    if (line.empty()) continue;
    if (line.front() == '#') continue;

    auto it = line.find(':');
    if (it == std::string::npos) continue;

    auto key = line.substr(0, it);
    auto value = line.substr(it + 1);
    if (key == "role") {
      if (value == "master") {
        ret.is_master = true;
      }
    } else if (key == "master_sync_in_progress") {
      if (value == "1") {
        ret.is_syncing = true;
      }
    } else if (key == "slave_read_repl_offset") {
      ret.slave_read_repl_offset = std::stoull(value);
    } else if (key == "slave_repl_offset") {
      ret.slave_repl_offset = std::stoull(value);
    }
  }
  return ret;
}

}  // namespace redis

USERVER_NAMESPACE_END
