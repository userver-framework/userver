#pragma once

/// @file
/// @brief @copybrief storages::redis::Config

#include <chrono>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/storages/redis/base.hpp>
#include <userver/storages/redis/wait_connected_mode.hpp>
#include <userver/utils/retry_budget.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

inline constexpr int kDeadlinePropagationExperimentVersion = 1;

CommandControl Parse(const formats::json::Value& elem, formats::parse::To<CommandControl>);

CommandControl::Strategy Parse(const formats::json::Value& elem, formats::parse::To<CommandControl::Strategy>);

WaitConnectedMode Parse(const formats::json::Value& elem, formats::parse::To<WaitConnectedMode>);

RedisWaitConnected Parse(const formats::json::Value& elem, formats::parse::To<RedisWaitConnected>);

CommandsBufferingSettings Parse(const formats::json::Value& elem, formats::parse::To<CommandsBufferingSettings>);

MetricsSettings::DynamicSettings
Parse(const formats::json::Value& elem, formats::parse::To<MetricsSettings::DynamicSettings>);

ReplicationMonitoringSettings
Parse(const formats::json::Value& elem, formats::parse::To<ReplicationMonitoringSettings>);

PubsubMetricsSettings Parse(const formats::json::Value& elem, formats::parse::To<PubsubMetricsSettings>);

/// @brief Main config for the Valkey/Redis
struct Config final {
    static Config Parse(const dynamic_config::DocsMap& docs_map);

    CommandControl default_command_control;
    CommandControl subscriber_default_command_control;
    std::chrono::seconds subscriptions_rebalance_min_interval{};
    RedisWaitConnected redis_wait_connected;
    CommandsBufferingSettings commands_buffering_settings;
    MetricsSettings::DynamicSettings metrics_settings;
    PubsubMetricsSettings pubsub_metrics_settings;
    dynamic_config::ValueDict<ReplicationMonitoringSettings> replication_monitoring_settings;
    dynamic_config::ValueDict<USERVER_NAMESPACE::utils::RetryBudgetSettings> retry_budget_settings;
};

extern const dynamic_config::Key<Config> kConfig;

}  // namespace storages::redis

USERVER_NAMESPACE_END
