#include <userver/utest/utest.hpp>

#include <chrono>
#include <ranges>
#include <string_view>
#include <vector>

#include <gmock/gmock.h>

#include <userver/dynamic_config/impl/snapshot.hpp>
#include <userver/dynamic_config/registered_config_meta.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/storages/redis/redis_config.hpp>
#include <userver/utils/algo.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kRedisConfigNames[]{
    "REDIS_DEFAULT_COMMAND_CONTROL",
    "REDIS_SUBSCRIBER_DEFAULT_COMMAND_CONTROL",
    "REDIS_SUBSCRIPTIONS_REBALANCE_MIN_INTERVAL_SECONDS",
    "REDIS_WAIT_CONNECTED",
    "REDIS_COMMANDS_BUFFERING_SETTINGS",
    "REDIS_METRICS_SETTINGS",
    "REDIS_PUBSUB_METRICS_SETTINGS",
    "REDIS_REPLICA_MONITORING_SETTINGS",
    "REDIS_RETRY_BUDGET_SETTINGS",
    "REDIS_IGNORE_HEALTH_CHECK",
};

UTEST(RedisConfig, CompositeConfigDefaultsAndMetadata) {
    const auto& config = dynamic_config::GetDefaultSnapshot()[storages::redis::kConfig];
    EXPECT_EQ(config.subscriptions_rebalance_min_interval, std::chrono::seconds{30});
    EXPECT_FALSE(config.ignore_health_check);

    const auto registered_configs = dynamic_config::impl::GetRegisteredConfigsMeta();
    for (const auto expected_name : kRedisConfigNames) {
        SCOPED_TRACE(expected_name);
        auto matching_configs =
            registered_configs |
            std::views::filter([expected_name](const auto& metadata) { return metadata.name == expected_name; });
        const auto metadata = utils::AsContainer<std::vector<dynamic_config::RegisteredConfigMeta>>(matching_configs);
        EXPECT_THAT(metadata, testing::Not(testing::IsEmpty()));
        EXPECT_THAT(
            metadata,
            testing::Each(testing::Field(
                "schema_hash",
                &dynamic_config::RegisteredConfigMeta::schema_hash,
                testing::Not(testing::IsEmpty())
            ))
        );
    }
}

}  // namespace

TEST(RedisCommandControlConfig, AllFieldsAreFilled) {
    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["timeout_all_ms"] = std::chrono::milliseconds{100}.count();
    builder["timeout_single_ms"] = std::chrono::milliseconds{200}.count();
    builder["max_retries"] = 3;
    builder["strategy"] = "every_dc";
    builder["best_dc_count"] = 4;
    builder["max_ping_latency_ms"] = std::chrono::milliseconds{500}.count();
    builder["allow_reads_from_master"] = true;
    builder["force_request_to_master"] = true;
    builder["consider_ping"] = true;
    builder["account_in_statistics"] = true;
    builder["force_shard_idx"] = 1;
    builder["chunk_size"] = 10;
    builder["force_retries_to_master_on_nil_reply"] = true;
    builder["retry_counter"] = 2;

    storages::redis::CommandControl config{};
    try {
        config = builder.ExtractValue().As<storages::redis::CommandControl>();
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    EXPECT_TRUE(config.timeout_all.has_value());
    EXPECT_EQ(config.timeout_all.value(), std::chrono::milliseconds{100});

    EXPECT_TRUE(config.timeout_single.has_value());
    EXPECT_EQ(config.timeout_single.value(), std::chrono::milliseconds{200});

    EXPECT_TRUE(config.max_retries.has_value());
    EXPECT_EQ(config.max_retries.value(), 3);

    EXPECT_TRUE(config.strategy.has_value());
    EXPECT_EQ(config.strategy.value(), storages::redis::CommandControl::Strategy::kEveryDc);

    EXPECT_TRUE(config.best_dc_count.has_value());
    EXPECT_EQ(config.best_dc_count.value(), 4);

    EXPECT_TRUE(config.max_ping_latency.has_value());
    EXPECT_EQ(config.max_ping_latency.value(), std::chrono::milliseconds{500});

    EXPECT_TRUE(config.allow_reads_from_master.has_value());
    EXPECT_EQ(config.allow_reads_from_master.value(), true);

    EXPECT_TRUE(config.force_request_to_master.has_value());
    EXPECT_EQ(config.force_request_to_master.value(), true);

    EXPECT_TRUE(config.consider_ping.has_value());
    EXPECT_EQ(config.consider_ping.value(), true);

    EXPECT_TRUE(config.account_in_statistics.has_value());
    EXPECT_EQ(config.account_in_statistics.value(), true);

    EXPECT_TRUE(config.force_shard_idx.has_value());
    EXPECT_EQ(config.force_shard_idx.value(), 1);

    EXPECT_TRUE(config.chunk_size.has_value());
    EXPECT_EQ(config.chunk_size.value(), 10);

    EXPECT_TRUE(config.force_retries_to_master_on_nil_reply);

    EXPECT_EQ(config.retry_counter, 2);
}

TEST(RedisCommandControlConfig, BareMinimum) {
    formats::json::ValueBuilder builder{formats::common::Type::kObject};

    storages::redis::CommandControl config{};
    try {
        config = builder.ExtractValue().As<storages::redis::CommandControl>();
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    EXPECT_FALSE(config.timeout_all.has_value());
    EXPECT_FALSE(config.timeout_single.has_value());
    EXPECT_FALSE(config.max_retries.has_value());
    EXPECT_FALSE(config.strategy.has_value());
    EXPECT_FALSE(config.best_dc_count.has_value());
    EXPECT_FALSE(config.max_ping_latency.has_value());
    EXPECT_FALSE(config.allow_reads_from_master.has_value());
    EXPECT_FALSE(config.force_request_to_master.has_value());
    EXPECT_FALSE(config.consider_ping.has_value());
    EXPECT_FALSE(config.account_in_statistics.has_value());
    EXPECT_FALSE(config.force_shard_idx.has_value());
    EXPECT_FALSE(config.chunk_size.has_value());
    EXPECT_FALSE(config.force_retries_to_master_on_nil_reply);
    EXPECT_EQ(config.retry_counter, 0);
}

TEST(RedisPubsubMetricsConfig, PerChannelStatsEnabled) {
    formats::json::ValueBuilder builder{formats::common::Type::kObject};
    builder["per-shard-stats-enabled"] = false;

    storages::redis::PubsubMetricsSettings config{};
    try {
        config = builder.ExtractValue().As<storages::redis::PubsubMetricsSettings>();
    } catch (const std::exception& e) {
        FAIL() << e.what();
    }

    EXPECT_FALSE(config.per_shard_stats_enabled);
}

USERVER_NAMESPACE_END
