#include <storages/postgres/postgres_config.hpp>

#include <fmt/format.h>
#include <optional>

#include <userver/formats/parse/common_containers.hpp>
#include <userver/logging/log.hpp>

#include <storages/postgres/experiments.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/exceptions.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/userver_info.hpp>

#include <userver/formats/common/items.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

CommandControl Parse(const formats::json::Value& elem, formats::parse::To<CommandControl>) {
    CommandControl result{components::Postgres::kDefaultCommandControl};
    for (const auto& [name, val] : formats::common::Items(elem)) {
        if (name == "network_timeout_ms") {
            const auto ms = std::chrono::milliseconds{val.As<std::int64_t>()};
            result.network_timeout_ms = ms;
            if (result.network_timeout_ms.count() <= 0) {
                throw InvalidConfig{
                    "Invalid network_timeout_ms `" + std::to_string(result.network_timeout_ms.count()) +
                    "` in postgres CommandControl. The timeout must be "
                    "greater than 0."
                };
            }
        } else if (name == "statement_timeout_ms") {
            const auto ms = std::chrono::milliseconds{val.As<std::int64_t>()};
            result.statement_timeout_ms = ms;
            if (result.statement_timeout_ms.count() <= 0) {
                throw InvalidConfig{
                    "Invalid statement_timeout_ms `" + std::to_string(result.statement_timeout_ms.count()) +
                    "` in postgres CommandControl. The timeout must be "
                    "greater than 0."
                };
            }
        } else if (name == "prepared_statements_enabled") {
            result.prepared_statements_enabled =
                val.As<bool>()
                    ? CommandControl::PreparedStatementsOptionOverride::kEnabled
                    : CommandControl::PreparedStatementsOptionOverride::kDisabled;
        } else {
            LOG_WARNING() << "Unknown parameter " << name << " in PostgreSQL config";
        }
    }
    return result;
}

namespace {

constexpr USERVER_NAMESPACE::utils::TrivialBiMap kPoolerModes = [](auto&& selector) {
    return selector().Case(PoolerMode::kSession, "session").Case(PoolerMode::kTransaction, "transaction");
};

PoolerMode ParsePoolerMode(const std::string_view pooler_mode_name) {
    const auto pooler_mode = kPoolerModes.TryFind(pooler_mode_name);
    if (!pooler_mode) {
        throw std::runtime_error("Unknown pooler mode: " + std::string{pooler_mode_name});
    }
    return *pooler_mode;
}

template <typename ConfigType>
ConnectionSettings::StatementLogMode ParseStatementLogMode(const ConfigType& config) {
    auto mode = config.template As<std::string>("show");
    if (mode == "hide") {
        return ConnectionSettings::kLogSkip;
    }
    if (mode == "show") {
        return ConnectionSettings::kLog;
    }
    throw std::runtime_error("Unknown statement log mode: " + mode);
}

template <typename ConfigType>
ConnectionSettings ParseConnectionSettings(const ConfigType& config) {
    ConnectionSettings settings{};
    settings.prepared_statements =
        config["persistent-prepared-statements"].template As<bool>(true)
            ? ConnectionSettings::kCachePreparedStatements
            : ConnectionSettings::kNoPreparedStatements;
    settings.statement_log_mode = ParseStatementLogMode(config["statement-log-mode"]);
    settings.user_types =
        config["user-types-enabled"].template As<bool>(true)
            ? config["check-user-types"].template As<bool>(false)
                  ? ConnectionSettings::kUserTypesEnforced
                  : ConnectionSettings::kUserTypesEnabled
            : ConnectionSettings::kPredefinedTypesOnly;
    // TODO: use hyphens in config keys, TAXICOMMON-5606
    settings.max_prepared_cache_size =
        config["max-prepared-cache-size"]
            .template As<size_t>(config["max_prepared_cache_size"].template As<size_t>(kDefaultMaxPreparedCacheSize));
    settings.ignore_unused_query_params =
        config["ignore-unused-query-params"]
                .template As<bool>(config["ignore_unused_query_params"].template As<bool>(false))
            ? ConnectionSettings::kIgnoreUnused
            : ConnectionSettings::kCheckUnused;

    settings.recent_errors_threshold =
        config["recent-errors-threshold"].template As<size_t>(settings.recent_errors_threshold);

    settings.max_ttl = config["max-ttl-sec"].template As<std::optional<std::chrono::seconds>>();

    settings.discard_on_connect =
        config["discard-all-on-connect"].template As<bool>(true)
            ? ConnectionSettings::kDiscardAll
            : ConnectionSettings::kDiscardNone;
    settings.deadline_propagation_enabled = config["deadline-propagation-enabled"].template As<bool>(true);
    settings.pooler_mode = config["pooler-mode"].template As<PoolerMode>(PoolerMode::kSession);
    settings.application_name =
        config["application_name"].template As<std::string>(USERVER_NAMESPACE::utils::GetUserverIdentifier());

    return settings;
}

}  // namespace

PoolerMode Parse(const yaml_config::YamlConfig& config, formats::parse::To<PoolerMode>) {
    return ParsePoolerMode(config.As<std::string>("session"));
}

PoolerMode Parse(const formats::json::Value& config, formats::parse::To<PoolerMode>) {
    return ParsePoolerMode(config.As<std::string>("session"));
}

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
        max_prepared_cache_size)
    {
        settings.max_prepared_cache_size = *max_prepared_cache_size;
    }
    if (const auto recent_errors_threshold = config["recent-errors-threshold"].As<std::optional<std::size_t>>();
        recent_errors_threshold)
    {
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
    if (const auto pooler_mode = config["pooler-mode"].As<std::optional<PoolerMode>>(); pooler_mode) {
        settings.pooler_mode = *pooler_mode;
    }

    return settings;
}

ConnectionSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<ConnectionSettings>) {
    return ParseConnectionSettings(config);
}

namespace {

void ValidatePoolSizes(const std::optional<std::size_t>& min_size, const std::optional<std::size_t>& max_size) {
    if (max_size == 0) {
        throw InvalidConfig{"max_pool_size must be greater than 0"};
    }
    if (max_size.has_value() && min_size.has_value() && *max_size < *min_size) {
        throw InvalidConfig{fmt::format(
            "max_pool_size cannot be less than min_pool_size. max_pool_size={}, min_pool_size={}",
            *max_size,
            *min_size
        )};
    }
}

template <typename T>
void MergeField(T& field, const std::optional<T>& opt) {
    if (opt) {
        field = *opt;
    }
}

}  // namespace

PoolSettingsDynamic Parse(const formats::json::Value& config, formats::parse::To<PoolSettingsDynamic>) {
    PoolSettingsDynamic result{
        .min_size = config["min_pool_size"].As<std::optional<std::size_t>>(),
        .max_size = config["max_pool_size"].As<std::optional<std::size_t>>(),
        .max_queue_size = config["max_queue_size"].As<std::optional<std::size_t>>(),
        .connecting_limit = config["connecting_limit"].As<std::optional<std::size_t>>(),
        .connecting_interval_ms = config["connecting_interval_ms"].As<std::optional<std::size_t>>(),
    };
    ValidatePoolSizes(result.min_size, result.max_size);
    return result;
}

PoolSettings Parse(const yaml_config::YamlConfig& config, formats::parse::To<PoolSettings>) {
    PoolSettings result{};
    result.min_size = config["min_pool_size"].As<std::size_t>(result.min_size);
    result.max_size = config["max_pool_size"].As<std::size_t>(result.max_size);
    result.max_queue_size = config["max_queue_size"].As<std::size_t>(result.max_queue_size);
    result.connecting_limit = config["connecting_limit"].As<std::size_t>(result.connecting_limit);

    const std::size_t default_connecting_interval_ms =
        USERVER_NAMESPACE::utils::impl::kPgConnectingRateLimitExperiment.IsEnabled()
            ? kExperimentDefaultConnectingIntervalMs
            : kDefaultConnectingIntervalMs;
    result.connecting_interval_ms = config["connecting_interval_ms"].As<std::size_t>(default_connecting_interval_ms);

    ValidatePoolSizes(result.min_size, result.max_size);
    return result;
}

void MergePoolSettings(const std::optional<PoolSettingsDynamic>& dynamic_settings, PoolSettings& static_settings) {
    if (!dynamic_settings.has_value()) {
        return;
    }
    const auto& dynamic = *dynamic_settings;
    MergeField(static_settings.max_size, dynamic.max_size);
    MergeField(static_settings.min_size, dynamic.min_size);
    MergeField(static_settings.max_queue_size, dynamic.max_queue_size);
    MergeField(static_settings.connecting_limit, dynamic.connecting_limit);
    MergeField(static_settings.connecting_interval_ms, dynamic.connecting_interval_ms);
}

TopologySettings Parse(const formats::json::Value& config, formats::parse::To<TopologySettings>) {
    TopologySettings result{};

    result.max_replication_lag =
        config["max_replication_lag_ms"].template As<std::chrono::milliseconds>(result.max_replication_lag);
    result.disabled_replicas = config["disabled_replicas"].template As<decltype(result.disabled_replicas)>({});

    if (result.max_replication_lag < std::chrono::milliseconds{0}) {
        throw InvalidConfig{"max_replication_lag cannot be less than 0"};
    }

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
        .default_command_control = docs_map.Get("POSTGRES_DEFAULT_COMMAND_CONTROL").As<CommandControl>(),
        .handlers_command_control = docs_map.Get("POSTGRES_HANDLERS_COMMAND_CONTROL").As<CommandControlByHandlerMap>(),
        .queries_command_control = docs_map.Get("POSTGRES_QUERIES_COMMAND_CONTROL").As<CommandControlByQueryMap>(),
        .pool_settings =
            docs_map.Get("POSTGRES_CONNECTION_POOL_SETTINGS").As<dynamic_config::ValueDict<PoolSettingsDynamic>>(),
        .topology_settings =
            docs_map.Get("POSTGRES_TOPOLOGY_SETTINGS").As<dynamic_config::ValueDict<TopologySettings>>(),
        .connection_settings =
            docs_map.Get("POSTGRES_CONNECTION_SETTINGS").As<dynamic_config::ValueDict<ConnectionSettingsDynamic>>(),
        .statement_metrics_settings =
            docs_map.Get("POSTGRES_STATEMENT_METRICS_SETTINGS")
                .As<dynamic_config::ValueDict<StatementMetricsSettings>>(),
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
