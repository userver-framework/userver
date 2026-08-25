#include <userver/storages/odbc/component.hpp>

#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/storages/odbc/cluster.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>

#include "odbc_secdist.hpp"

#include <userver/storages/odbc/command_control.hpp>

#include <dynamic_config/variables/USERVER_ODBC_CONNECTION_POOL_SETTINGS.hpp>
#include <dynamic_config/variables/USERVER_ODBC_DEFAULT_COMMAND_CONTROL.hpp>
#include <dynamic_config/variables/USERVER_ODBC_HANDLERS_COMMAND_CONTROL.hpp>
#include <dynamic_config/variables/USERVER_ODBC_PREPARED_STATEMENT_CACHE_SETTINGS.hpp>
#include <dynamic_config/variables/USERVER_ODBC_QUERIES_COMMAND_CONTROL.hpp>
#include <dynamic_config/variables/USERVER_ODBC_STATEMENT_METRICS_SETTINGS.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/odbc/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

void ValidatePoolSettings(const storages::odbc::settings::PoolSettings& settings) {
    UINVARIANT(settings.max_size > 0, "ODBC max_pool_size must be positive");
    UINVARIANT(settings.min_size <= settings.max_size, "ODBC min_pool_size must not exceed max_pool_size");
}

void ValidateNonEmpty(std::string_view value, std::string_view option) {
    if (value.empty()) {
        throw std::runtime_error("ODBC component option '" + std::string{option} + "' must not be empty");
    }
}

engine::TaskProcessor& GetBlockingTaskProcessor(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) {
    const auto name = config["blocking_task_processor"].As<std::optional<std::string>>();
    return name ? context.GetTaskProcessor(*name) : engine::current_task::GetBlockingTaskProcessor();
}

storages::odbc::settings::ODBCClusterSettings MakeClusterSettingsFromConfig(const components::ComponentConfig& config) {
    using storages::odbc::settings::HostSettings;
    using storages::odbc::settings::ODBCClusterSettings;
    using storages::odbc::settings::PoolSettings;

    if (const auto dsn_opt = config["dsn"].As<std::optional<std::string>>(); dsn_opt.has_value()) {
        ValidateNonEmpty(*dsn_opt, "dsn");
        const auto min_size = config["min_pool_size"].As<std::size_t>(PoolSettings{}.min_size);
        const auto max_size = config["max_pool_size"].As<std::size_t>(PoolSettings{}.max_size);
        ValidatePoolSettings(PoolSettings{.min_size = min_size, .max_size = max_size});
        return ODBCClusterSettings{std::vector<HostSettings>{
            HostSettings{.dsn = *dsn_opt, .pool = {.min_size = min_size, .max_size = max_size}},
        }};
    }

    const auto pools_cfg = config["pools"];
    if (!pools_cfg.IsMissing() && pools_cfg.GetSize() > 0) {
        std::vector<HostSettings> pools;
        pools.reserve(pools_cfg.GetSize());
        for (std::size_t i = 0; i < pools_cfg.GetSize(); ++i) {
            const auto pool = pools_cfg[i];
            const auto dsn = pool["dsn"].As<std::string>();
            ValidateNonEmpty(dsn, "pools[" + std::to_string(i) + "].dsn");
            const auto min_size = pool["min_pool_size"].As<std::size_t>(PoolSettings{}.min_size);
            const auto max_size = pool["max_pool_size"].As<std::size_t>(PoolSettings{}.max_size);
            ValidatePoolSettings(PoolSettings{.min_size = min_size, .max_size = max_size});
            pools.emplace_back(HostSettings{
                .dsn = dsn,
                .pool = {.min_size = min_size, .max_size = max_size},
            });
        }
        return ODBCClusterSettings{std::move(pools)};
    }

    return ODBCClusterSettings{};
}

storages::odbc::settings::ODBCClusterSettings MakeClusterSettingsFromSecdist(
    const storages::odbc::secdist::OdbcSettings& odbc_settings,
    const std::string& dbalias,
    const components::ComponentConfig& config
) {
    using storages::odbc::settings::HostSettings;
    using storages::odbc::settings::ODBCClusterSettings;
    using storages::odbc::settings::PoolSettings;

    const auto connection_infos = odbc_settings.GetConnectionInfos(dbalias);

    const auto min_size = config["min_pool_size"].As<std::size_t>(PoolSettings{}.min_size);
    const auto max_size = config["max_pool_size"].As<std::size_t>(PoolSettings{}.max_size);
    ValidatePoolSettings(PoolSettings{.min_size = min_size, .max_size = max_size});

    std::vector<HostSettings> pools;
    pools.reserve(connection_infos.size());
    for (const auto& info : connection_infos) {
        pools.emplace_back(HostSettings{
            .dsn = info.dsn,
            .pool = {.min_size = min_size, .max_size = max_size},
        });
    }

    return ODBCClusterSettings{std::move(pools)};
}

storages::odbc::settings::ODBCClusterSettings MakeClusterSettings(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) {
    const auto secdist_alias = config["secdist_alias"].As<std::optional<std::string>>();
    const auto dsn = config["dsn"].As<std::optional<std::string>>();
    const auto pools = config["pools"];
    const auto has_pools = !pools.IsMissing() && pools.GetSize() > 0;

    const auto connection_sources =
        static_cast<unsigned>(secdist_alias.has_value()) + static_cast<unsigned>(dsn.has_value()) +
        static_cast<unsigned>(has_pools);
    UINVARIANT(
        connection_sources == 1,
        "Exactly one ODBC connection source must be configured: 'dsn', 'pools', or 'secdist_alias'"
    );

    if (secdist_alias.has_value()) {
        ValidateNonEmpty(*secdist_alias, "secdist_alias");
        const auto& secdist = context.FindComponent<components::Secdist>();
        const auto& odbc_settings = secdist.Get().Get<storages::odbc::secdist::OdbcSettings>();
        return MakeClusterSettingsFromSecdist(odbc_settings, *secdist_alias, config);
    }

    return MakeClusterSettingsFromConfig(config);
}

template <typename ConfigCommandControl>
storages::odbc::CommandControl ConvertCommandControl(const ConfigCommandControl& command_control) {
    return {
        .network_timeout = command_control.network_timeout_ms,
        .statement_timeout = command_control.statement_timeout_ms,
    };
}

template <typename ConfigMap>
storages::odbc::CommandControlByQueryMap ConvertQueriesCommandControl(const ConfigMap& config) {
    storages::odbc::CommandControlByQueryMap result;
    result.reserve(config.extra.size());
    for (const auto& [name, command_control] : config.extra) {
        result.emplace(name, ConvertCommandControl(command_control));
    }
    return result;
}

template <typename ConfigMap>
storages::odbc::CommandControlByHandlerMap ConvertHandlersCommandControl(const ConfigMap& config) {
    storages::odbc::CommandControlByHandlerMap result;
    result.reserve(config.extra.size());
    for (const auto& [path, config_by_method] : config.extra) {
        storages::odbc::CommandControlByMethodMap by_method;
        by_method.reserve(config_by_method.extra.size());
        for (const auto& [method, command_control] : config_by_method.extra) {
            by_method.emplace(method, ConvertCommandControl(command_control));
        }
        result.emplace(path, std::move(by_method));
    }
    return result;
}

}  // namespace

Odbc::Odbc(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase{config, context},
      name_{config.Name()},
      statement_metrics_settings_fallback_{
          .max_statements = config["max_statement_metrics"].As<std::size_t>(0),
      },
      secdist_alias_{config["secdist_alias"].As<std::optional<std::string>>()},
      cluster_{std::make_shared<storages::odbc::Cluster>(
          MakeClusterSettings(config, context),
          clients::dns::GetResolverPtr(config, context),
          GetBlockingTaskProcessor(config, context)
      )},
      config_source_{context.FindComponent<components::DynamicConfig>().GetSource()}
{
    cluster_->SetStatementMetricsSettings(statement_metrics_settings_fallback_);
    cluster_->SetPreparedStatementCacheSettings({
        .max_size = config["max_prepared_cache_size"].As<std::size_t>(0),
    });

    utils::statistics::RegisterWriterScope(
        context,
        "odbc",
        [this](utils::statistics::Writer& writer) { cluster_->WriteStatistics(writer); },
        {{"component", name_}}
    );

    // Subscribe to dynamic config updates
    config_source_.UpdateAndListen(
        context.Scopes(),
        this,
        "odbc",
        &Odbc::OnConfigUpdate,
        ::dynamic_config::USERVER_ODBC_CONNECTION_POOL_SETTINGS,
        ::dynamic_config::USERVER_ODBC_DEFAULT_COMMAND_CONTROL,
        ::dynamic_config::USERVER_ODBC_HANDLERS_COMMAND_CONTROL,
        ::dynamic_config::USERVER_ODBC_QUERIES_COMMAND_CONTROL,
        ::dynamic_config::USERVER_ODBC_PREPARED_STATEMENT_CACHE_SETTINGS,
        ::dynamic_config::USERVER_ODBC_STATEMENT_METRICS_SETTINGS
    );

    if (secdist_alias_) {
        auto& secdist = context.FindComponent<components::Secdist>();
        secdist_subscription_ = secdist.GetStorage().UpdateAndListen(this, name_, &Odbc::OnSecdistUpdate);
    }
}

Odbc::~Odbc() = default;

void Odbc::OnConfigUpdate(const dynamic_config::Snapshot& config) {
    const auto& pool_settings = config[::dynamic_config::USERVER_ODBC_CONNECTION_POOL_SETTINGS];

    // Apply default command control from dynamic config
    const auto pool_settings_opt = pool_settings.GetOptional(name_);
    std::optional<storages::odbc::settings::PoolSettings> updated;
    if (pool_settings_opt.has_value()) {
        updated = storages::odbc::settings::PoolSettings{
            .min_size = pool_settings_opt->min_pool_size,
            .max_size = pool_settings_opt->max_pool_size,
        };
        ValidatePoolSettings(*updated);
    }
    cluster_->SetPoolSettingsOverride(updated);

    const auto& statement_metrics = config[::dynamic_config::USERVER_ODBC_STATEMENT_METRICS_SETTINGS];
    auto statement_metrics_settings = statement_metrics_settings_fallback_;
    if (const auto dynamic_settings = statement_metrics.GetOptional(name_)) {
        statement_metrics_settings.max_statements = dynamic_settings->max_statement_metrics;
    }
    cluster_->SetStatementMetricsSettings(statement_metrics_settings);

    const auto& prepared_statement_cache = config[::dynamic_config::USERVER_ODBC_PREPARED_STATEMENT_CACHE_SETTINGS];
    std::optional<storages::odbc::settings::PreparedStatementCacheSettings> prepared_statement_cache_override;
    if (const auto dynamic_settings = prepared_statement_cache.GetOptional(name_)) {
        prepared_statement_cache_override = {
            .max_size = dynamic_settings->max_prepared_cache_size,
        };
    }
    cluster_->SetPreparedStatementCacheSettingsOverride(prepared_statement_cache_override);

    const auto& default_command_control = config[::dynamic_config::USERVER_ODBC_DEFAULT_COMMAND_CONTROL];
    const auto& handlers_command_control = config[::dynamic_config::USERVER_ODBC_HANDLERS_COMMAND_CONTROL];
    const auto& queries_command_control = config[::dynamic_config::USERVER_ODBC_QUERIES_COMMAND_CONTROL];
    cluster_->ApplyDynamicCommandControls(
        ConvertCommandControl(default_command_control),
        ConvertHandlersCommandControl(handlers_command_control),
        ConvertQueriesCommandControl(queries_command_control)
    );
}

void Odbc::OnSecdistUpdate(const storages::secdist::SecdistConfig& secdist) {
    UASSERT(secdist_alias_);
    const auto& odbc_settings = secdist.Get<storages::odbc::secdist::OdbcSettings>();
    const auto connection_infos = odbc_settings.GetConnectionInfos(*secdist_alias_);

    std::vector<std::string> dsns;
    dsns.reserve(connection_infos.size());
    for (const auto& info : connection_infos) {
        dsns.push_back(info.dsn);
    }
    cluster_->UpdateDsns(dsns);
}

std::shared_ptr<storages::odbc::Cluster> Odbc::GetCluster() const { return cluster_; }

yaml_config::Schema Odbc::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/odbc/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
