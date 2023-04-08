#include <storages/postgres/postgres_config.hpp>

#include <userver/logging/log.hpp>

#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

CommandControl Parse(const formats::json::Value& elem,
                     formats::parse::To<CommandControl>) {
  CommandControl result{components::Postgres::kDefaultCommandControl};
  for (auto it = elem.begin(); it != elem.end(); ++it) {
    const auto& name = it.GetName();
    if (name == "network_timeout_ms") {
      result.execute = std::chrono::milliseconds{it->As<int64_t>()};
      if (result.execute.count() <= 0) {
        throw InvalidConfig{"Invalid network_timeout_ms `" +
                            std::to_string(result.execute.count()) +
                            "` in postgres CommandControl. The timeout must be "
                            "greater than 0."};
      }
    } else if (name == "statement_timeout_ms") {
      result.statement = std::chrono::milliseconds{it->As<int64_t>()};
      if (result.statement.count() <= 0) {
        throw InvalidConfig{"Invalid statement_timeout_ms `" +
                            std::to_string(result.statement.count()) +
                            "` in postgres CommandControl. The timeout must be "
                            "greater than 0."};
      }
    } else {
      LOG_WARNING() << "Unknown parameter " << name << " in PostgreSQL config";
    }
  }
  return result;
}

namespace {

template <typename ConfigType>
ConnectionSettings ParseConnectionSettings(const ConfigType& config) {
  ConnectionSettings settings{};
  settings.prepared_statements =
      config["persistent-prepared-statements"].template As<bool>(true)
          ? ConnectionSettings::kCachePreparedStatements
          : ConnectionSettings::kNoPreparedStatements;
  settings.user_types = config["user-types-enabled"].template As<bool>(true)
                            ? ConnectionSettings::kUserTypesEnabled
                            : ConnectionSettings::kPredefinedTypesOnly;
  // TODO: use hyphens in config keys, TAXICOMMON-5606
  settings.max_prepared_cache_size =
      config["max-prepared-cache-size"].template As<size_t>(
          config["max_prepared_cache_size"].template As<size_t>(
              kDefaultMaxPreparedCacheSize));
  settings.ignore_unused_query_params =
      config["ignore-unused-query-params"].template As<bool>(
          config["ignore_unused_query_params"].template As<bool>(false))
          ? ConnectionSettings::kIgnoreUnused
          : ConnectionSettings::kCheckUnused;

  settings.recent_errors_threshold =
      config["recent-errors-threshold"].template As<size_t>(
          settings.recent_errors_threshold);
  return settings;
}

}  // namespace

ConnectionSettings Parse(const formats::json::Value& config,
                         formats::parse::To<ConnectionSettings>) {
  return ParseConnectionSettings(config);
}

ConnectionSettings Parse(const yaml_config::YamlConfig& config,
                         formats::parse::To<ConnectionSettings>) {
  return ParseConnectionSettings(config);
}

namespace {

template <typename ConfigType>
PoolSettings ParsePoolSettings(const ConfigType& config) {
  PoolSettings result{};
  result.min_size =
      config["min_pool_size"].template As<size_t>(result.min_size);
  result.max_size =
      config["max_pool_size"].template As<size_t>(result.max_size);
  result.max_queue_size =
      config["max_queue_size"].template As<size_t>(result.max_queue_size);
  result.connecting_limit =
      config["connecting_limit"].template As<size_t>(result.connecting_limit);

  if (result.max_size == 0)
    throw InvalidConfig{"max_pool_size must be greater than 0"};
  if (result.max_size < result.min_size)
    throw InvalidConfig{"max_pool_size cannot be less than min_pool_size"};

  return result;
}

}  // namespace

PoolSettings Parse(const formats::json::Value& config,
                   formats::parse::To<PoolSettings>) {
  return ParsePoolSettings(config);
}

PoolSettings Parse(const yaml_config::YamlConfig& config,
                   formats::parse::To<PoolSettings>) {
  return ParsePoolSettings(config);
}

namespace {

template <typename ConfigType>
StatementMetricsSettings ParseStatementMetricsSettings(
    const ConfigType& config) {
  StatementMetricsSettings result{};
  result.max_statements = config["max_statement_metrics"].template As<size_t>(
      result.max_statements);

  return result;
}

}  // namespace

StatementMetricsSettings Parse(const formats::json::Value& config,
                               formats::parse::To<StatementMetricsSettings>) {
  return ParseStatementMetricsSettings(config);
}

StatementMetricsSettings Parse(const yaml_config::YamlConfig& config,
                               formats::parse::To<StatementMetricsSettings>) {
  return ParseStatementMetricsSettings(config);
}

PipelineMode ParsePipelineMode(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("POSTGRES_CONNECTION_PIPELINE_ENABLED").As<bool>(false)
             ? PipelineMode::kEnabled
             : PipelineMode::kDisabled;
}

Config::Config(const dynamic_config::DocsMap& docs_map)
    : default_command_control{"POSTGRES_DEFAULT_COMMAND_CONTROL", docs_map},
      handlers_command_control{"POSTGRES_HANDLERS_COMMAND_CONTROL", docs_map},
      queries_command_control{"POSTGRES_QUERIES_COMMAND_CONTROL", docs_map},
      pool_settings{"POSTGRES_CONNECTION_POOL_SETTINGS", docs_map},
      connection_settings{"POSTGRES_CONNECTION_SETTINGS", docs_map},
      statement_metrics_settings("POSTGRES_STATEMENT_METRICS_SETTINGS",
                                 docs_map) {}

ConnlimitConfig ParseConnlimitConfig(const dynamic_config::DocsMap& docs_map) {
  return {docs_map.Get("POSTGRES_CONNLIMIT_MODE_AUTO_ENABLED").As<bool>()};
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
