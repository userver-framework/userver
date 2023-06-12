#pragma once

/// @file userver/storages/mysql/cluster_host_type.hpp

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

/// @brief Enum for selecting the host type to carry query execution.
enum class ClusterHostType {
  /// Execute a query on primary host.
  kPrimary,
  /// Execute a query on a secondary (replica) host.
  /// Fallbacks to primary in standalone topology.
  kSecondary
};

}  // namespace storages::mysql

USERVER_NAMESPACE_END
