#pragma once

#include <string>

USERVER_NAMESPACE_BEGIN

namespace redis {

/// Replication info of redis instance
struct ReplicationInfo {
  /// True if "role:master"
  bool is_master = false;
  /// True if "master_sync_in_progress:1"
  bool is_syncing = false;
  size_t slave_read_repl_offset = 0;
  size_t slave_repl_offset = 0;
};

/// Parse replication info from response of redis "INFO REPLICATION" command
ReplicationInfo ParseReplicationInfo(const std::string& info);

}  // namespace redis

USERVER_NAMESPACE_END
