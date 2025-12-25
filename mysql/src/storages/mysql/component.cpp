#include <userver/storages/mysql/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>

#include <storages/mysql/settings/settings.hpp>

#include <userver/storages/mysql/cluster.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/storages/mysql/component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace {

std::shared_ptr<storages::mysql::Cluster> CreateCluster(
    clients::dns::Resolver& resolver,
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) {
    const auto& secdist = context.FindComponent<components::Secdist>().Get();
    const auto& settings_multi = secdist.Get<storages::mysql::settings::MysqlSettingsMulti>();
    const auto& settings = settings_multi.Get(storages::mysql::settings::GetSecdistAlias(config));

    return std::make_shared<storages::mysql::Cluster>(resolver, settings, config);
}

}  // namespace

Component::Component(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()},
      cluster_{CreateCluster(dns_.GetResolver(), config, context)}
{
    auto& statistics_storage = context.FindComponent<components::StatisticsStorage>();
    statistics_holder_ =
        statistics_storage.GetStorage().RegisterWriter("mysql", [this](utils::statistics::Writer& writer) {
            cluster_->WriteStatistics(writer);
        });
}

Component::~Component() { statistics_holder_.Unregister(); }

std::shared_ptr<storages::mysql::Cluster> Component::GetCluster() const { return cluster_; }

yaml_config::Schema Component::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/storages/mysql/component.yaml");
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
