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
    config.enable_subscriptions = value["enable_subscriptions"].As<bool>(false);
    config.per_channel_stats_enabled = value["per_channel_statistics"].As<bool>(true);
    config.topology_update_method =
        storages::redis::ToTopologyUpdateMethod(value["topology_update_method"].As<std::string>("cluster_slots"));
    config.required_mode = storages::redis::Parse(
        value["required"].As<std::string>("no_wait"),
        formats::parse::To<storages::redis::WaitConnectedMode>()
    );
    config.max_disconnect_time = value["max_disconnect_time"].As<std::chrono::seconds>(kDefaultMaxDisconnectTime);
    if (value["max_failed_shards"].IsObject()) {
        config.max_failed_shards = value["max_failed_shards"]["amount"].As<uint32_t>(0);
        config.max_failed_shards_percent = value["max_failed_shards"]["percent"].As<uint32_t>(0);
    }
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
    config.required_mode = storages::redis::Parse(
        value["required"].As<std::string>("no_wait"),
        formats::parse::To<storages::redis::WaitConnectedMode>()
    );
    config.max_disconnect_time = value["max_disconnect_time"].As<std::chrono::seconds>(kDefaultMaxDisconnectTime);
    if (value["max_failed_shards"].IsObject()) {
        config.max_failed_shards = value["max_failed_shards"]["amount"].As<uint32_t>(0);
        config.max_failed_shards_percent = value["max_failed_shards"]["percent"].As<uint32_t>(0);
    }
    return config;
}

bool CanShareSentinel(const RedisGroup& group, const SubscribeRedisGroup& subscribe_group) {
    if (group.db != subscribe_group.db || group.config_name != subscribe_group.config_name ||
        group.sharding_strategy != subscribe_group.sharding_strategy ||
        group.topology_update_method != subscribe_group.topology_update_method ||
        group.max_disconnect_time != subscribe_group.max_disconnect_time)
    {
        return false;
    }
    // Cluster topology shares failure hysteresis, which is enabled for required clients.
    const bool required = group.required_mode != WaitConnectedMode::kNoWait;
    const bool subscribe_required = subscribe_group.required_mode != WaitConnectedMode::kNoWait;
    return group.sharding_strategy != ShardingStrategy::kRedisCluster || required == subscribe_required;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
