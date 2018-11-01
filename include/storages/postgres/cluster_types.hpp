#pragma once

#include <iosfwd>
#include <unordered_map>

#include <storages/postgres/dsn.hpp>

namespace storages {
namespace postgres {

enum class ClusterHostType {
  /// Connect to cluster's master. Only this connection may be
  /// used for read-write transactions.
  kMaster = 0x01,
  /// Connect to cluster's sync slave. May fallback to master. Can be used only
  /// for read only transactions.
  kSyncSlave = 0x02,
  /// Connect to one of cluster's slaves. May fallback to sync
  /// slave.  Can be used only for read only transactions.
  kSlave = 0x04,
  /// Any available
  kAny = kMaster | kSyncSlave | kSlave
};

std::ostream& operator<<(std::ostream&, const ClusterHostType&);
std::string ToString(const ClusterHostType& ht);

/// @brief Cluster configuration description
struct ClusterDescription {
  /// Master replica DSN
  std::string master_dsn_;

  /// Sync slave replica DSN
  std::string sync_slave_dsn_;

  /// List of async slave replica DSNs
  DSNList slave_dsns_;

  ClusterDescription() = default;
  explicit ClusterDescription(const std::string& multi_host_dsn);
  ClusterDescription(const std::string& master_dsn,
                     const std::string& sync_slave_dsn, DSNList slave_dsns);
};

using ShardedClusterDescription =
    std::unordered_map<size_t, ClusterDescription>;

}  // namespace postgres
}  // namespace storages
