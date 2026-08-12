#include <userver/storages/odbc/component.hpp>

#include <optional>
#include <vector>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/storages/odbc/cluster.hpp>
#include <userver/storages/odbc/settings.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/dynamic_config/snapshot.hpp>
#include <userver/dynamic_config/value.hpp>

#include "odbc_config.hpp"
#include "odbc_secdist.hpp"

#include <dynamic_config/variables/USERVER_ODBC_CONNECTION_POOL_SETTINGS.hpp>
#include <dynamic_config/variables/USERVER_ODBC_DEFAULT_COMMAND_CONTROL.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/odbc/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

storages::odbc::settings::ODBCClusterSettings MakeClusterSettingsFromConfig(const components::ComponentConfig& config) {
    using storages::odbc::settings::HostSettings;
    using storages::odbc::settings::ODBCClusterSettings;
    using storages::odbc::settings::PoolSettings;

    if (const auto dsn_opt = config["dsn"].As<std::optional<std::string>>(); dsn_opt.has_value()) {
        const auto min_size = config["min_pool_size"].As<std::size_t>(PoolSettings{}.min_size);
        const auto max_size = config["max_pool_size"].As<std::size_t>(PoolSettings{}.max_size);
        return ODBCClusterSettings{std::vector<HostSettings>{
            HostSettings{*dsn_opt, PoolSettings{min_size, max_size}},
        }};
    }

    const auto pools_cfg = config["pools"];
    if (!pools_cfg.IsMissing() && pools_cfg.GetSize() > 0) {
        std::vector<HostSettings> pools;
        pools.reserve(pools_cfg.GetSize());
        for (std::size_t i = 0; i < pools_cfg.GetSize(); ++i) {
            const auto pool = pools_cfg[i];
            const auto dsn = pool["dsn"].As<std::string>();
            const auto min_size = pool["min_pool_size"].As<std::size_t>(PoolSettings{}.min_size);
            const auto max_size = pool["max_pool_size"].As<std::size_t>(PoolSettings{}.max_size);
            pools.emplace_back(HostSettings{dsn, PoolSettings{min_size, max_size}});
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

    std::vector<HostSettings> pools;
    pools.reserve(connection_infos.size());
    for (const auto& info : connection_infos) {
        pools.emplace_back(HostSettings{info.dsn, PoolSettings{min_size, max_size}});
    }

    return ODBCClusterSettings{std::move(pools)};
}

storages::odbc::settings::ODBCClusterSettings MakeClusterSettings(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) {
    const auto secdist_alias = config["secdist_alias"].As<std::optional<std::string>>();

    if (secdist_alias.has_value()) {
        const auto& secdist = context.FindComponent<components::Secdist>();
        const auto& odbc_settings = secdist.Get().Get<storages::odbc::secdist::OdbcSettings>();
        return MakeClusterSettingsFromSecdist(odbc_settings, *secdist_alias, config);
    }

    auto settings = MakeClusterSettingsFromConfig(config);
    UINVARIANT(!settings.pools.empty(), "Either 'dsn', 'pools', or 'secdist_alias' must be set");
    return settings;
}

}  // namespace

Odbc::Odbc(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase{config, context},
      name_{config.Name()},
      cluster_{std::make_shared<
          storages::odbc::Cluster>(MakeClusterSettings(config, context), clients::dns::GetResolverPtr(config, context))
      },
      config_source_{context.FindComponent<components::DynamicConfig>().GetSource()}
{
    utils::statistics::RegisterWriterScope(
        context,
        "odbc",
        [this](utils::statistics::Writer& writer) { cluster_->WriteStatistics(writer); },
        {{"component", name_}}
    );

    // Subscribe to dynamic config updates
    config_subscription_ = config_source_.UpdateAndListen(
        this,
        "odbc",
        &Odbc::OnConfigUpdate,
        ::dynamic_config::USERVER_ODBC_CONNECTION_POOL_SETTINGS,
        ::dynamic_config::USERVER_ODBC_DEFAULT_COMMAND_CONTROL
    );
}

Odbc::~Odbc() { config_subscription_.Unsubscribe(); }

void Odbc::OnConfigUpdate(const dynamic_config::Snapshot& config) {
    const auto& pool_settings = config[::dynamic_config::USERVER_ODBC_CONNECTION_POOL_SETTINGS];

    // Apply default command control from dynamic config
    const auto pool_settings_opt = pool_settings.GetOptional(name_);
    if (pool_settings_opt.has_value()) {
        // Note: Pool size changes require restart as ConnectionPoolBase
        // doesn't support dynamic resizing. Log a warning if settings differ.
        // In future versions, this could be enhanced to support dynamic resizing.
    }

    // Apply command control (timeouts)
    const auto cc = config[::dynamic_config::USERVER_ODBC_DEFAULT_COMMAND_CONTROL];
    cluster_->SetDefaultCommandControl(storages::odbc::CommandControl{
        .network_timeout = cc.network_timeout_ms,
        .statement_timeout = cc.statement_timeout_ms,
    });
}

std::shared_ptr<storages::odbc::Cluster> Odbc::GetCluster() const { return cluster_; }

yaml_config::Schema Odbc::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/odbc/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END
