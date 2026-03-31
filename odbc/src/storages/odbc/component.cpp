#include <userver/storages/odbc/component.hpp>

#include <optional>
#include <vector>

#include <userver/clients/dns/resolver_utils.hpp>
#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/utils/assert.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/odbc/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

storages::odbc::settings::ODBCClusterSettings MakeClusterSettings(const components::ComponentConfig& config) {
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
    UINVARIANT(!pools_cfg.IsMissing() && pools_cfg.GetSize() > 0, "Either 'dsn' or non-empty 'pools' must be set");

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

}  // namespace

Odbc::Odbc(const ComponentConfig& config, const ComponentContext& context)
    : ComponentBase{config, context},
      cluster_{std::make_shared<storages::odbc::Cluster>(
          MakeClusterSettings(config),
          clients::dns::GetResolverPtr(config, context)
      )}
{
    auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();
    statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
        "odbc",
        [this](utils::statistics::Writer& writer) { cluster_->WriteStatistics(writer); },
        {{"component", config.Name()}}
    );
}

Odbc::~Odbc() { statistics_holder_.Unregister(); }

std::shared_ptr<storages::odbc::Cluster> Odbc::GetCluster() const { return cluster_; }

yaml_config::Schema Odbc::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/odbc/component.yaml");
}

}  // namespace components

USERVER_NAMESPACE_END

