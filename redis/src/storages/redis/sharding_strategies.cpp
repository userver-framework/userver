#include <userver/storages/redis/sharding_strategies.hpp>

#include <string>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {
ShardingStrategy ToShardingStrategy(std::string_view view) {
    if (view == "RedisCluster") {
        return ShardingStrategy::kRedisCluster;
    }
    if (view == "RedisStandalone") {
        return ShardingStrategy::kRedisStandalone;
    }
    if (view == "KeyShardCrc32") {
        return ShardingStrategy::kKeyShardCrc32;
    }
    if (view == "KeyShardTaximeterCrc32") {
        return ShardingStrategy::kKeyShardTaximeterCrc32;
    }
    if (view == "KeyShardGpsStorageDriver") {
        return ShardingStrategy::kKeyShardGpsStorageDriver;
    }
    throw std::invalid_argument("Unknown sharding strategy '" + std::string(view) + "'");
}

std::string_view ToStringView(ShardingStrategy value) {
    switch (value) {
        case ShardingStrategy::kRedisCluster:
            return "RedisCluster";
        case ShardingStrategy::kRedisStandalone:
            return "RedisStandalone";
        case ShardingStrategy::kKeyShardCrc32:
            return "KeyShardCrc32";
        case ShardingStrategy::kKeyShardTaximeterCrc32:
            return "KeyShardTaximeterCrc32";
        case ShardingStrategy::kKeyShardGpsStorageDriver:
            return "KeyShardGpsStorageDriver";
    }
    UASSERT(false);
    return "Unknown";
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
