#pragma once

/// @file userver/storages/redis/sharding_strategies.hpp
/// @brief @copybrief storages::redis::ShardingStrategy

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

/// @brief How Redis keys are mapped to shards or cluster topology
enum class ShardingStrategy {
    kRedisCluster,
    kRedisStandalone,
    kKeyShardCrc32,
    kKeyShardTaximeterCrc32,
    kKeyShardGpsStorageDriver
};

ShardingStrategy ToShardingStrategy(std::string_view view);
std::string_view ToStringView(ShardingStrategy);

}  // namespace storages::redis

USERVER_NAMESPACE_END
