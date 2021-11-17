#pragma once

#include <userver/taxi_config/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>);

PoolSettings Parse(const formats::json::Value& config,
                   formats::parse::To<PoolSettings>);

PoolSettings Parse(const yaml_config::YamlConfig& config,
                   formats::parse::To<PoolSettings>);

StatementMetricsSettings Parse(const formats::json::Value& config,
                               formats::parse::To<StatementMetricsSettings>);

StatementMetricsSettings Parse(const yaml_config::YamlConfig& config,
                               formats::parse::To<StatementMetricsSettings>);

class Config {
 public:
  taxi_config::Value<CommandControl> default_command_control;
  taxi_config::Value<CommandControlByHandlerMap> handlers_command_control;
  taxi_config::Value<CommandControlByQueryMap> queries_command_control;
  taxi_config::ValueDict<PoolSettings> pool_settings;
  taxi_config::ValueDict<StatementMetricsSettings> statement_metrics_settings;

  Config(const taxi_config::DocsMap& docs_map);
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
