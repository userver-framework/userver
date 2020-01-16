#pragma once

/// @file storages/postgres/cluster_types.hpp
/// @brief Cluster properties

#include <iosfwd>
#include <vector>

#include <boost/variant.hpp>

#include <storages/postgres/dsn.hpp>
#include <storages/postgres/exceptions.hpp>

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

inline ClusterHostType Fallback(ClusterHostType ht) {
  switch (ht) {
    case ClusterHostType::kMaster:
      throw ClusterError("Cannot fallback from master");
    case ClusterHostType::kSyncSlave:
      return ClusterHostType::kMaster;
    case ClusterHostType::kSlave:
      return ClusterHostType::kSyncSlave;
    case ClusterHostType::kAny:
      throw ClusterError("Invalid ClusterHostType value for fallback " +
                         std::to_string(static_cast<int>(ht)));
  }
}

std::ostream& operator<<(std::ostream&, const ClusterHostType&);
std::string ToString(const ClusterHostType& ht);

/// @brief Cluster configuration description
struct ClusterDescription {
  // Will be deprecated as soon as all migrate to the new secdist scheme
  struct PredefinedRoles {
    /// Master replica DSN
    std::string master_dsn_;

    /// Sync slave replica DSN
    std::string sync_slave_dsn_;

    /// List of async slave replica DSNs
    DSNList slave_dsns_;
  };
  enum TopologyType { kAuto, kPredefined };
  using Description = boost::variant<DSNList, PredefinedRoles>;

  Description description_;

  ClusterDescription() = default;
  explicit ClusterDescription(DSNList multi_host_dsn);
  ClusterDescription(const std::string& master_dsn,
                     const std::string& sync_slave_dsn, DSNList slave_dsns);

  TopologyType GetTopologyType() const {
    return static_cast<TopologyType>(description_.which());
  }
};

using ShardedClusterDescription = std::vector<ClusterDescription>;

}  // namespace postgres
}  // namespace storages
