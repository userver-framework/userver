#pragma once

#include <optional>
#include <string>

#include <userver/formats/parse/to.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/health_check_param.hpp>
#include <userver/storages/redis/sharding_strategies.hpp>
#include <userver/storages/redis/topology_update_method.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <storages/redis/impl/connection_kind.hpp>
#include <storages/redis/impl/keyshard.hpp>
#include <storages/redis/impl/redis_connection_defaults.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

/// @brief Configuration parsed from a `groups` entry in the static config.
///
/// Holds the per-database options that originate from the component config and
/// are eventually forwarded into the Sentinel / SentinelImpl constructor chain.
struct RedisGroup {
    /// Name of the client / database (used as the lookup key in `clients_` map).
    std::string db;
    /// Key in the secdist secrets map used to look up connection settings.
    std::string config_name;
    storages::redis::ShardingStrategy sharding_strategy{storages::redis::ShardingStrategy::kKeyShardTaximeterCrc32};
    bool allow_reads_from_master{false};
    storages::redis::TopologyUpdateMethod topology_update_method{storages::redis::TopologyUpdateMethod::kClusterSlots};
    storages::redis::WaitConnectedMode required_mode{storages::redis::WaitConnectedMode::kNoWait};
    uint32_t max_failed_shards{0};
    uint32_t max_failed_shards_percent{0};
    std::chrono::seconds max_disconnect_time{kDefaultMaxDisconnectTime};
    bool enable_subscriptions{false};
    bool per_channel_stats_enabled{true};
};

RedisGroup Parse(const yaml_config::YamlConfig& value, formats::parse::To<RedisGroup>);

/// @brief Configuration parsed from a `subscribe_groups` entry in the static config.
///
/// Extends RedisGroup with subscription-specific options.
struct SubscribeRedisGroup {
    std::string db;
    std::string config_name;
    storages::redis::ShardingStrategy sharding_strategy{storages::redis::ShardingStrategy::kKeyShardTaximeterCrc32};
    bool allow_reads_from_master{false};
    storages::redis::TopologyUpdateMethod topology_update_method{storages::redis::TopologyUpdateMethod::kClusterSlots};
    bool per_channel_stats_enabled{true};
    storages::redis::WaitConnectedMode required_mode{storages::redis::WaitConnectedMode::kNoWait};
    uint32_t max_failed_shards{0};
    uint32_t max_failed_shards_percent{0};
    std::chrono::seconds max_disconnect_time{kDefaultMaxDisconnectTime};
};

SubscribeRedisGroup Parse(const yaml_config::YamlConfig& value, formats::parse::To<SubscribeRedisGroup>);

/// Whether both clients can share topology without changing their connection settings.
bool CanShareSentinel(const RedisGroup& group, const SubscribeRedisGroup& subscribe_group);

/// @brief Bundle of parameters that originate from RedisGroup and are threaded
///        through the Sentinel / SentinelImpl constructor chain.
///
/// Using this struct instead of individual positional arguments makes it easier
/// to add new parameters without changing all intermediate call sites.
struct SentinelStaticConfig {
    /// Name used to identify this client in logs and statistics.
    std::string client_name;
    /// Factory that creates the key-sharding implementation.
    KeyShardFactory key_shard_factory;
    /// Default command control (e.g. allow_reads_from_master).
    CommandControl command_control;
    /// Method used to discover topology updates.
    storages::redis::TopologyUpdateMethod topology_update_method{storages::redis::TopologyUpdateMethod::kClusterSlots};

    storages::redis::WaitConnectedMode required_mode{storages::redis::WaitConnectedMode::kNoWait};
    uint32_t max_failed_shards{0};
    uint32_t max_failed_shards_percent{0};
    std::chrono::seconds max_disconnect_time{kDefaultMaxDisconnectTime};
    ConnectionMode connection_mode{ConnectionMode::kCommands};
    bool per_channel_stats_enabled{true};
    std::optional<CommandControl> subscription_command_control{};
};

/// @brief Helper: build a SentinelStaticConfig from a RedisGroup.
inline SentinelStaticConfig MakeSentinelStaticConfig(
    const RedisGroup& group,
    const SubscribeRedisGroup* subscribe_group = nullptr
) {
    CommandControl cc{};
    cc.allow_reads_from_master = group.allow_reads_from_master;
    auto config = SentinelStaticConfig{
        group.db,
        KeyShardFactory{group.sharding_strategy},
        cc,
        group.topology_update_method,
        group.required_mode,
        group.max_failed_shards,
        group.max_failed_shards_percent,
        group.max_disconnect_time,
        group.enable_subscriptions ? ConnectionMode::kCommandsAndSubscriptions : ConnectionMode::kCommands,
        group.per_channel_stats_enabled
    };
    if (subscribe_group) {
        config.connection_mode = ConnectionMode::kCommandsAndSubscriptions;
        config.per_channel_stats_enabled = subscribe_group->per_channel_stats_enabled;
        config.subscription_command_control.emplace()
            .allow_reads_from_master = subscribe_group->allow_reads_from_master;
    }
    return config;
}

/// @brief Build the configuration for a subscription group.
inline SentinelStaticConfig MakeSentinelStaticConfig(const SubscribeRedisGroup& group) {
    CommandControl cc{};
    cc.allow_reads_from_master = group.allow_reads_from_master;
    return SentinelStaticConfig{
        group.db,
        KeyShardFactory{group.sharding_strategy},
        cc,
        group.topology_update_method,
        group.required_mode,
        group.max_failed_shards,
        group.max_failed_shards_percent,
        group.max_disconnect_time,
        ConnectionMode::kSubscriptions,
        group.per_channel_stats_enabled
    };
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
