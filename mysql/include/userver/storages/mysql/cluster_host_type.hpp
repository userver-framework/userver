#pragma once

/// @file userver/storages/mysql/cluster_host_type.hpp

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

/// @brief Enum for selecting the host type to carry query execution.
enum class ClusterHostType {
  /// Execute a query on master host.
  kMaster,
  /// Execute a query on a slave host.
  /// Fallbacks to master in standalone topology.
  kSlave
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
