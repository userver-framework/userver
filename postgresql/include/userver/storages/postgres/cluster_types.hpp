#pragma once

/// @file userver/storages/postgres/cluster_types.hpp
/// @brief Cluster properties

#include <string>

#include <userver/utils/flags.hpp>

USERVER_NAMESPACE_BEGIN

namespace logging {
class LogHelper;
}  // namespace logging

namespace storages::postgres {

enum class ClusterHostType {
  /// @name Cluster roles
  /// @{
  /// No host role detected yet.
  kNone = 0x00,
  /// Connect to cluster's master. Only this connection may be
  /// used for read-write transactions.
  kMaster = 0x01,
  /// Connect to cluster's sync slave. May fallback to master. Can be used only
  /// for read only transactions.
  /// @warning Not available for clusters with quorum commit,
  /// prefer kSlave or kMaster.
  kSyncSlave = 0x02,
  /// Connect to one of cluster's slaves. May fallback to master. Can be used
  /// only for read only transactions.
  kSlave = 0x04,
  /// Connect to either a master or to a slave, whatever the host selection
  /// strategy chooses. Can be used only for read only transactions.
  kSlaveOrMaster = kMaster | kSlave,
  /// @}

  /// @name Host selection strategies
  /// @{

  /// Chooses a host using the round-robin algorithm
  kRoundRobin = 0x08,

  /// Chooses a host with the lowest RTT
  kNearest = 0x10,
  /// @}
};

using ClusterHostTypeFlags = USERVER_NAMESPACE::utils::Flags<ClusterHostType>;

constexpr ClusterHostTypeFlags kClusterHostRolesMask{
    ClusterHostType::kMaster, ClusterHostType::kSyncSlave,
    ClusterHostType::kSlave};

constexpr ClusterHostTypeFlags kClusterHostStrategyMask{
    ClusterHostType::kRoundRobin, ClusterHostType::kNearest};

std::string ToString(ClusterHostType);
std::string ToString(ClusterHostTypeFlags);
logging::LogHelper& operator<<(logging::LogHelper&, ClusterHostType);
logging::LogHelper& operator<<(logging::LogHelper&, ClusterHostTypeFlags);

struct ClusterHostTypeHash {
  size_t operator()(ClusterHostType ht) const noexcept {
    return static_cast<size_t>(ht);
  }
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
