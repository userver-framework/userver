#pragma once

/// @file userver/storages/clickhouse/fwd.hpp
/// @brief Forward declarations of some popular clickhouse related types

#include <memory>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

class Cluster;

using ClusterPtr = std::shared_ptr<Cluster>;

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
