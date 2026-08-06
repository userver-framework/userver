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
    config.required_mode = storages::redis::Parse(
        value["required"].As<std::string>("no_wait"),
        formats::parse::To<storages::redis::WaitConnectedMode>()
    );
    config.max_disconnect_time = value["max_disconnect_time"].As<std::chrono::seconds>(std::chrono::seconds(35));
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
    config.max_disconnect_time = value["max_disconnect_time"].As<std::chrono::seconds>(std::chrono::seconds(35));
    if (value["max_failed_shards"].IsObject()) {
        config.max_failed_shards = value["max_failed_shards"]["amount"].As<uint32_t>(0);
        config.max_failed_shards_percent = value["max_failed_shards"]["percent"].As<uint32_t>(0);
    }
    return config;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
