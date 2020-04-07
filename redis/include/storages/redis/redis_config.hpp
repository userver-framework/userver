#pragma once

#include <formats/parse/to.hpp>
#include <storages/redis/impl/base.hpp>
#include <storages/redis/impl/wait_connected_mode.hpp>
#include <taxi_config/value.hpp>

namespace redis {
CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>);

CommandControl::Strategy Parse(const formats::json::Value& elem,
                               formats::parse::To<CommandControl::Strategy>);

WaitConnectedMode Parse(const formats::json::Value& elem,
                        formats::parse::To<WaitConnectedMode>);

RedisWaitConnected Parse(const formats::json::Value& elem,
                         formats::parse::To<RedisWaitConnected>);

}  // namespace redis

namespace storages {
namespace redis {

class Config {
 public:
  taxi_config::Value<::redis::CommandControl> default_command_control;
  taxi_config::Value<::redis::CommandControl>
      subscriber_default_command_control;
  taxi_config::Value<size_t> subscriptions_rebalance_min_interval_seconds;
  taxi_config::Value<::redis::RedisWaitConnected> redis_wait_connected;

  Config(const taxi_config::DocsMap& docs_map)
      : default_command_control{"REDIS_DEFAULT_COMMAND_CONTROL", docs_map},
        subscriber_default_command_control{
            "REDIS_SUBSCRIBER_DEFAULT_COMMAND_CONTROL", docs_map},
        subscriptions_rebalance_min_interval_seconds{
            "REDIS_SUBSCRIPTIONS_REBALANCE_MIN_INTERVAL_SECONDS", docs_map},
        redis_wait_connected{"REDIS_WAIT_CONNECTED", docs_map} {}
};

}  // namespace redis
}  // namespace storages
