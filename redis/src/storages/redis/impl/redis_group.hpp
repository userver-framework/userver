#pragma once

#include <string>

#include <userver/formats/parse/to.hpp>
#include <userver/storages/redis/command_options.hpp>
#include <userver/storages/redis/sharding_strategies.hpp>
#include <userver/storages/redis/topology_update_method.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <storages/redis/impl/keyshard.hpp>

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
};

SubscribeRedisGroup Parse(const yaml_config::YamlConfig& value, formats::parse::To<SubscribeRedisGroup>);

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
};

/// @brief Extends SentinelStaticConfig with subscription-specific options.
struct SubscribeSentinelStaticConfig {
    std::string client_name;
    KeyShardFactory key_shard_factory;
    CommandControl command_control;
    storages::redis::TopologyUpdateMethod topology_update_method{storages::redis::TopologyUpdateMethod::kClusterSlots};
    bool per_channel_stats_enabled{true};
};

/// @brief Helper: build a SentinelStaticConfig from a RedisGroup.
inline SentinelStaticConfig MakeSentinelStaticConfig(const RedisGroup& group) {
    CommandControl cc{};
    cc.allow_reads_from_master = group.allow_reads_from_master;
    return SentinelStaticConfig{
        group.db,
        KeyShardFactory{group.sharding_strategy},
        cc,
        group.topology_update_method,
    };
}

/// @brief Helper: build a SubscribeSentinelStaticConfig from a SubscribeRedisGroup.
inline SubscribeSentinelStaticConfig MakeSubscribeSentinelStaticConfig(const SubscribeRedisGroup& group) {
    CommandControl cc{};
    cc.allow_reads_from_master = group.allow_reads_from_master;
    return SubscribeSentinelStaticConfig{
        group.db,
        KeyShardFactory{group.sharding_strategy},
        cc,
        group.topology_update_method,
        group.per_channel_stats_enabled,
    };
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
