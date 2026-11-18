#include "redis_group.hpp"
#include <storages/redis/impl/connection_kind.hpp>

#include <chrono>
#include <initializer_list>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {
namespace {

RedisGroup MakeGroup(ShardingStrategy strategy = ShardingStrategy::kKeyShardTaximeterCrc32) {
    RedisGroup group;
    group.db = "shared";
    group.config_name = "shared-secdist";
    group.sharding_strategy = strategy;
    return group;
}

SubscribeRedisGroup MakeSubscribeGroup(ShardingStrategy strategy = ShardingStrategy::kKeyShardTaximeterCrc32) {
    SubscribeRedisGroup group;
    group.db = "shared";
    group.config_name = "shared-secdist";
    group.sharding_strategy = strategy;
    return group;
}

TEST(RedisGroup, MatchingGroupsShareSentinelWithoutExplicitFlag) {
    for (const auto strategy :
         {ShardingStrategy::kKeyShardTaximeterCrc32, ShardingStrategy::kRedisStandalone, ShardingStrategy::kRedisCluster
         })
    {
        auto group = MakeGroup(strategy);
        const auto subscribe_group = MakeSubscribeGroup(strategy);
        EXPECT_TRUE(CanShareSentinel(group, subscribe_group));
        EXPECT_EQ(
            MakeSentinelStaticConfig(group, &subscribe_group).connection_mode,
            ConnectionMode::kCommandsAndSubscriptions
        );
        group.enable_subscriptions = true;
        EXPECT_TRUE(CanShareSentinel(group, subscribe_group));
    }
}

TEST(RedisGroup, DifferentDatabaseOrSecdistCannotShareSentinel) {
    const auto group = MakeGroup();
    auto subscribe_group = MakeSubscribeGroup();
    subscribe_group.db = "other";
    EXPECT_FALSE(CanShareSentinel(group, subscribe_group));
    subscribe_group = MakeSubscribeGroup();
    subscribe_group.config_name = "other-secdist";
    EXPECT_FALSE(CanShareSentinel(group, subscribe_group));
}

TEST(RedisGroup, DifferentConnectionSettingsCannotShareSentinel) {
    const auto group = MakeGroup();
    auto subscribe_group = MakeSubscribeGroup(ShardingStrategy::kRedisStandalone);
    EXPECT_FALSE(CanShareSentinel(group, subscribe_group));
    subscribe_group = MakeSubscribeGroup();
    subscribe_group.topology_update_method = TopologyUpdateMethod::kClusterShards;
    EXPECT_FALSE(CanShareSentinel(group, subscribe_group));
    subscribe_group = MakeSubscribeGroup();
    subscribe_group.max_disconnect_time += std::chrono::seconds{1};
    EXPECT_FALSE(CanShareSentinel(group, subscribe_group));
}

TEST(RedisGroup, ClusterFailureHysteresisMustMatch) {
    auto group = MakeGroup(ShardingStrategy::kRedisCluster);
    auto subscribe_group = MakeSubscribeGroup(ShardingStrategy::kRedisCluster);
    subscribe_group.required_mode = WaitConnectedMode::kMaster;
    EXPECT_FALSE(CanShareSentinel(group, subscribe_group));
    group.required_mode = WaitConnectedMode::kSlave;
    EXPECT_TRUE(CanShareSentinel(group, subscribe_group));
    subscribe_group.required_mode = WaitConnectedMode::kNoWait;
    EXPECT_FALSE(CanShareSentinel(group, subscribe_group));
}

TEST(RedisGroup, SubscriptionSettingsRemainIndependent) {
    auto group = MakeGroup();
    group.allow_reads_from_master = true;
    group.required_mode = WaitConnectedMode::kMaster;
    group.max_failed_shards = 1;
    auto subscribe_group = MakeSubscribeGroup();
    subscribe_group.required_mode = WaitConnectedMode::kSlave;
    subscribe_group.max_failed_shards_percent = 50;
    subscribe_group.per_channel_stats_enabled = false;
    ASSERT_TRUE(CanShareSentinel(group, subscribe_group));

    const auto config = MakeSentinelStaticConfig(group, &subscribe_group);
    EXPECT_EQ(config.command_control.allow_reads_from_master, true);
    ASSERT_TRUE(config.subscription_command_control);
    EXPECT_EQ(config.subscription_command_control->allow_reads_from_master, false);
    EXPECT_FALSE(config.per_channel_stats_enabled);
    EXPECT_EQ(config.required_mode, WaitConnectedMode::kMaster);
    EXPECT_EQ(config.max_failed_shards, 1);
    EXPECT_EQ(config.max_failed_shards_percent, 0);
}

TEST(RedisGroup, ExplicitSubscriptionsKeepGroupSettings) {
    auto group = MakeGroup();
    group.enable_subscriptions = true;
    group.allow_reads_from_master = true;
    group.per_channel_stats_enabled = false;
    const auto config = MakeSentinelStaticConfig(group);
    EXPECT_EQ(config.connection_mode, ConnectionMode::kCommandsAndSubscriptions);
    EXPECT_FALSE(config.subscription_command_control);
    EXPECT_EQ(config.command_control.allow_reads_from_master, true);
    EXPECT_FALSE(config.per_channel_stats_enabled);
}

}  // namespace
}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
