#pragma once

/// @file userver/storages/redis/topology_update_method.hpp
/// @brief @copybrief storages::redis::TopologyUpdateMethod

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief How Redis Cluster topology refresh is performed
enum class TopologyUpdateMethod { kClusterSlots, kClusterShards };

TopologyUpdateMethod ToTopologyUpdateMethod(std::string_view view);
std::string_view ToStringView(TopologyUpdateMethod);

}  // namespace storages::redis

USERVER_NAMESPACE_END
