#pragma once

/// @file storages/postgres/cluster_types.hpp
/// @brief Cluster properties

#include <iosfwd>

#include <storages/postgres/exceptions.hpp>

namespace storages {
namespace postgres {

enum class ClusterHostType {
  /// No host role detected yet
  kUnknown = 0x00,
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
    case ClusterHostType::kUnknown:
    case ClusterHostType::kAny:
      throw ClusterError("Invalid ClusterHostType value for fallback " +
                         std::to_string(static_cast<int>(ht)));
  }
}

std::ostream& operator<<(std::ostream&, const ClusterHostType&);
std::string ToString(const ClusterHostType& ht);

struct ClusterHostTypeHash {
  size_t operator()(ClusterHostType ht) const noexcept {
    return static_cast<size_t>(ht);
  }
};

}  // namespace postgres
}  // namespace storages
