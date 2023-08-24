#pragma once

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <userver/storages/postgres/options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>);

ConnectionSettings Parse(const formats::json::Value& config,
                         formats::parse::To<ConnectionSettings>);

ConnectionSettings Parse(const yaml_config::YamlConfig& config,
                         formats::parse::To<ConnectionSettings>);

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
  dynamic_config::Value<CommandControl> default_command_control;
  dynamic_config::Value<CommandControlByHandlerMap> handlers_command_control;
  dynamic_config::Value<CommandControlByQueryMap> queries_command_control;
  dynamic_config::ValueDict<PoolSettings> pool_settings;
  dynamic_config::ValueDict<ConnectionSettings> connection_settings;
  dynamic_config::ValueDict<StatementMetricsSettings>
      statement_metrics_settings;

  Config(const dynamic_config::DocsMap& docs_map);
};

PipelineMode ParsePipelineMode(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParsePipelineMode> kPipelineModeKey;

class ConnlimitConfig {
 public:
  bool connlimit_mode_auto_enabled;
};

ConnlimitConfig ParseConnlimitConfig(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseConnlimitConfig> kConnlimitConfig;

int ParseDeadlinePropagation(const dynamic_config::DocsMap& docs_map);

inline constexpr dynamic_config::Key<ParseDeadlinePropagation>
    kDeadlinePropagationVersionConfig;

}  // namespace storages::postgres

USERVER_NAMESPACE_END
