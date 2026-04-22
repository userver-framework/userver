#pragma once
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

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
