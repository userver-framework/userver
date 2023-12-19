#pragma once

#include <chrono>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/wait_connected_mode.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {
CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>);

CommandControl::Strategy Parse(const formats::json::Value& elem,
                               formats::parse::To<CommandControl::Strategy>);

WaitConnectedMode Parse(const formats::json::Value& elem,
                        formats::parse::To<WaitConnectedMode>);

RedisWaitConnected Parse(const formats::json::Value& elem,
                         formats::parse::To<RedisWaitConnected>);

CommandsBufferingSettings Parse(const formats::json::Value& elem,
                                formats::parse::To<CommandsBufferingSettings>);

MetricsSettings::DynamicSettings Parse(
    const formats::json::Value& elem,
    formats::parse::To<MetricsSettings::DynamicSettings>);

ReplicationMonitoringSettings Parse(
    const formats::json::Value& elem,
    formats::parse::To<ReplicationMonitoringSettings>);

PubsubMetricsSettings Parse(const formats::json::Value& elem,
                            formats::parse::To<PubsubMetricsSettings>);
}  // namespace redis

namespace storages::redis {

struct Config final {
  static Config Parse(const dynamic_config::DocsMap& docs_map);

  USERVER_NAMESPACE::redis::CommandControl default_command_control;
  USERVER_NAMESPACE::redis::CommandControl subscriber_default_command_control;
  std::chrono::seconds subscriptions_rebalance_min_interval{};
  USERVER_NAMESPACE::redis::RedisWaitConnected redis_wait_connected;
  USERVER_NAMESPACE::redis::CommandsBufferingSettings
      commands_buffering_settings;
  USERVER_NAMESPACE::redis::MetricsSettings::DynamicSettings metrics_settings;
  USERVER_NAMESPACE::redis::PubsubMetricsSettings pubsub_metrics_settings;
  dynamic_config::ValueDict<
      USERVER_NAMESPACE::redis::ReplicationMonitoringSettings>
      replication_monitoring_settings;
  bool redis_cluster_autotopology_enabled{};
};

extern const dynamic_config::Key<Config> kConfig;

}  // namespace storages::redis

USERVER_NAMESPACE_END
