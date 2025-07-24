#include <storages/postgres/postgres_config.hpp>

#include <userver/logging/log.hpp>

#include <storages/postgres/experiments.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>

#include <userver/formats/common/items.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

CommandControl Parse(const formats::json::Value& elem, formats::parse::To<CommandControl>) {
    CommandControl result{components::Postgres::kDefaultCommandControl};
    for (const auto& [name, val] : formats::common::Items(elem)) {
        const auto ms = std::chrono::milliseconds{val.As<std::int64_t>()};
        if (name == "network_timeout_ms") {
            result.network_timeout_ms = ms;
            if (result.network_timeout_ms.count() <= 0) {
                throw InvalidConfig{
                    "Invalid network_timeout_ms `" + std::to_string(result.network_timeout_ms.count()) +
                    "` in postgres CommandControl. The timeout must be "
                    "greater than 0."};
            }
        } else if (name == "statement_timeout_ms") {
            result.statement_timeout_ms = ms;
            if (result.statement_timeout_ms.count() <= 0) {
                throw InvalidConfig{
                    "Invalid statement_timeout_ms `" + std::to_string(result.statement_timeout_ms.count()) +
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
ConnectionSettings::StatementLogMode ParseStatementLogMode(const ConfigType& config) {
    auto mode = config.template As<std::string>("show");
    if (mode == "hide") return ConnectionSettings::kLogSkip;
    if (mode == "show") return ConnectionSettings::kLog;
    throw std::runtime_error("Unknown statement log mode: " + mode);
}

template <typename ConfigType>
ConnectionSettings ParseConnectionSettings(const ConfigType& config) {
    ConnectionSettings settings{};
    settings.prepared_statements = config["persistent-prepared-statements"].template As<bool>(true)
                                       ? ConnectionSettings::kCachePreparedStatements
                                       : ConnectionSettings::kNoPreparedStatements;
    settings.statement_log_mode = ParseStatementLogMode(config["statement-log-mode"]);
    settings.user_types = config["user-types-enabled"].template As<bool>(true)
                              ? config["check-user-types"].template As<bool>(false)
                                    ? ConnectionSettings::kUserTypesEnforced
                                    : ConnectionSettings::kUserTypesEnabled
                              : ConnectionSettings::kPredefinedTypesOnly;
    // TODO: use hyphens in config keys, TAXICOMMON-5606
    settings.max_prepared_cache_size = config["max-prepared-cache-size"].template As<size_t>(
        config["max_prepared_cache_size"].template As<size_t>(kDefaultMaxPreparedCacheSize)
    );
    settings.ignore_unused_query_params = config["ignore-unused-query-params"].template As<bool>(
                                              config["ignore_unused_query_params"].template As<bool>(false)
                                          )
                                              ? ConnectionSettings::kIgnoreUnused
                                              : ConnectionSettings::kCheckUnused;

    settings.recent_errors_threshold =
        config["recent-errors-threshold"].template As<size_t>(settings.recent_errors_threshold);

    settings.max_ttl = config["max-ttl-sec"].template As<std::optional<std::chrono::seconds>>();

    settings.discard_on_connect = config["discard-all-on-connect"].template As<bool>(true)
                                      ? ConnectionSettings::kDiscardAll
                                      : ConnectionSettings::kDiscardNone;
    settings.deadline_propagation_enabled = config["deadline-propagation-enabled"].template As<bool>(true);

    return settings;
}

}  // namespace

ConnectionSettings::StatementLogMode
Parse(const yaml_config::YamlConfig& config, formats::parse::To<ConnectionSettings::StatementLogMode>) {
    return ParseStatementLogMode(config);
}

ConnectionSettingsDynamic Parse(const formats::json::Value& config, formats::parse::To<ConnectionSettingsDynamic>) {
    ConnectionSettingsDynamic settings{};
    if (const auto pps = config["persistent-prepared-statements"].As<std::optional<bool>>(); pps) {
        settings.prepared_statements =
            *pps ? ConnectionSettings::kCachePreparedStatements : ConnectionSettings::kNoPreparedStatements;
    }
    if (const auto user_types_enabled = config["user-types-enabled"].As<std::optional<bool>>(); user_types_enabled) {
        if (*user_types_enabled) {
            if (const auto check = config["check-user-types"].As<std::optional<bool>>(); check) {
                settings.user_types =
                    *check ? ConnectionSettings::kUserTypesEnforced : ConnectionSettings::kUserTypesEnabled;
            }
        } else {
            settings.user_types = ConnectionSettings::kPredefinedTypesOnly;
        }
    }
    if (const auto max_prepared_cache_size = config["max-prepared-cache-size"].As<std::optional<std::size_t>>();
        max_prepared_cache_size) {
        settings.max_prepared_cache_size = *max_prepared_cache_size;
    }
    if (const auto recent_errors_threshold = config["recent-errors-threshold"].As<std::optional<std::size_t>>();
        recent_errors_threshold) {
        settings.recent_errors_threshold = *recent_errors_threshold;
    }
    if (const auto ignore = config["ignore-unused-query-params"].As<std::optional<bool>>(); ignore) {
        settings.ignore_unused_query_params =
            *ignore ? ConnectionSettings::kIgnoreUnused : ConnectionSettings::kCheckUnused;
    }
    if (const auto max_ttl = config["max-ttl-sec"].As<std::optional<std::chrono::seconds>>(); max_ttl) {
        settings.max_ttl = *max_ttl;
    }
    if (const auto discard_all = config["discard-all-on-connect"].As<std::optional<bool>>(); discard_all) {
        settings.discard_on_connect = *discard_all ? ConnectionSettings::kDiscardAll : ConnectionSettings::kDiscardNone;
    }
    if (const auto dp_enabled = config["deadline-propagation-enabled"].As<std::optional<bool>>(); dp_enabled) {
        settings.deadline_propagation_enabled = *dp_enabled;
    }

    return settings;
}

ConnectionSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<ConnectionSettings>) {
    return ParseConnectionSettings(config);
}

namespace {

template <typename T, typename ConfigType>
T GetField(const ConfigType& config, std::string_view name, T default_val) {
    return config[name].template As<T>(default_val);
}

template <typename Settings, typename ConfigType>
Settings ParsePoolSettings(const ConfigType& config) {
    Settings result{};
    result.min_size = GetField(config, "min_pool_size", result.min_size);
    result.max_size = GetField(config, "max_pool_size", result.max_size);
    result.max_queue_size = GetField(config, "max_queue_size", result.max_queue_size);
    result.connecting_limit = GetField(config, "connecting_limit", result.connecting_limit);

    if (result.max_size == 0) throw InvalidConfig{"max_pool_size must be greater than 0"};
    if (result.max_size < result.min_size) throw InvalidConfig{"max_pool_size cannot be less than min_pool_size"};

    return result;
}

}  // namespace

PoolSettingsDynamic Parse(const formats::json::Value& config, formats::parse::To<PoolSettingsDynamic>) {
    return ParsePoolSettings<PoolSettingsDynamic>(config);
}

PoolSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<PoolSettings>) {
    return ParsePoolSettings<PoolSettings>(config);
}

TopologySettings Parse(const formats::json::Value& config, formats::parse::To<TopologySettings>) {
    TopologySettings result{};

    result.max_replication_lag =
        config["max_replication_lag_ms"].template As<std::chrono::milliseconds>(result.max_replication_lag);
    result.disabled_replicas = config["disabled_replicas"].template As<decltype(result.disabled_replicas)>({});

    if (result.max_replication_lag < std::chrono::milliseconds{0})
        throw InvalidConfig{"max_replication_lag cannot be less than 0"};

    return result;
}

namespace {

template <typename ConfigType>
StatementMetricsSettings ParseStatementMetricsSettings(const ConfigType& config) {
    StatementMetricsSettings result{};
    result.max_statements = config["max_statement_metrics"].template As<size_t>(result.max_statements);

    return result;
}

}  // namespace

StatementMetricsSettings Parse(const formats::json::Value& config, formats::parse::To<StatementMetricsSettings>) {
    return ParseStatementMetricsSettings(config);
}

StatementMetricsSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<StatementMetricsSettings>) {
    return ParseStatementMetricsSettings(config);
}

Config Config::Parse(const dynamic_config::DocsMap& docs_map) {
    return Config{
        /*default_command_control=*/docs_map.Get("POSTGRES_DEFAULT_COMMAND_CONTROL").As<CommandControl>(),
        /*handlers_command_control=*/
        docs_map.Get("POSTGRES_HANDLERS_COMMAND_CONTROL").As<CommandControlByHandlerMap>(),
        /*queries_command_control=*/
        docs_map.Get("POSTGRES_QUERIES_COMMAND_CONTROL").As<CommandControlByQueryMap>(),
        /*pool_settings=*/
        docs_map.Get("POSTGRES_CONNECTION_POOL_SETTINGS").As<dynamic_config::ValueDict<PoolSettingsDynamic>>(),
        /*topology_settings*/
        docs_map.Get("POSTGRES_TOPOLOGY_SETTINGS").As<dynamic_config::ValueDict<TopologySettings>>(),
        /*connection_settings=*/
        docs_map.Get("POSTGRES_CONNECTION_SETTINGS").As<dynamic_config::ValueDict<ConnectionSettingsDynamic>>(),
        /*statement_metrics_settings=*/
        docs_map.Get("POSTGRES_STATEMENT_METRICS_SETTINGS").As<dynamic_config::ValueDict<StatementMetricsSettings>>(),
    };
}

using JsonString = dynamic_config::DefaultAsJsonString;

const dynamic_config::Key<Config> kConfig{
    Config::Parse,
    {
        {"POSTGRES_DEFAULT_COMMAND_CONTROL", JsonString{"{}"}},
        {"POSTGRES_HANDLERS_COMMAND_CONTROL", JsonString{"{}"}},
        {"POSTGRES_QUERIES_COMMAND_CONTROL", JsonString{"{}"}},
        {"POSTGRES_CONNECTION_POOL_SETTINGS", JsonString{"{}"}},
        {"POSTGRES_TOPOLOGY_SETTINGS", JsonString{"{}"}},
        {"POSTGRES_CONNECTION_SETTINGS", JsonString{"{}"}},
        {"POSTGRES_STATEMENT_METRICS_SETTINGS", JsonString{"{}"}},
    },
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
