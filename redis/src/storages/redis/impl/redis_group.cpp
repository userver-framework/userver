#include "redis_group.hpp"

#include <userver/logging/log.hpp>
#include <userver/storages/redis/sharding_strategies.hpp>
#include <userver/storages/redis/topology_update_method.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

RedisGroup Parse(const yaml_config::YamlConfig& value, formats::parse::To<RedisGroup>) {
    RedisGroup config;
    config.db = value["db"].As<std::string>();
    config.config_name = value["config_name"].As<std::string>();
    config.sharding_strategy =
        storages::redis::ToShardingStrategy(value["sharding_strategy"].As<std::string>("KeyShardTaximeterCrc32"));
    config.allow_reads_from_master = value["allow_reads_from_master"].As<bool>(false);
    config.topology_update_method =
        storages::redis::ToTopologyUpdateMethod(value["topology_update_method"].As<std::string>("cluster_slots"));
    return config;
}

SubscribeRedisGroup Parse(const yaml_config::YamlConfig& value, formats::parse::To<SubscribeRedisGroup>) {
    SubscribeRedisGroup config;
    config.db = value["db"].As<std::string>();
    config.config_name = value["config_name"].As<std::string>();
    config.sharding_strategy =
        storages::redis::ToShardingStrategy(value["sharding_strategy"].As<std::string>("KeyShardTaximeterCrc32"));
    config.allow_reads_from_master = value["allow_reads_from_master"].As<bool>(false);
    config.topology_update_method =
        storages::redis::ToTopologyUpdateMethod(value["topology_update_method"].As<std::string>("cluster_slots"));
    config.per_channel_stats_enabled = value["per_channel_statistics"].As<bool>(true);
    LOG_DEBUG() << "config.per_channel_stats_enabled=" << config.per_channel_stats_enabled;
    return config;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
