#include <userver/storages/mysql/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>

#include <storages/mysql/settings/settings.hpp>

#include <userver/storages/mysql/cluster.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace {

std::shared_ptr<storages::mysql::Cluster> CreateCluster(
    clients::dns::Resolver& resolver, const components::ComponentConfig& config,
    const components::ComponentContext& context) {
  const auto& secdist = context.FindComponent<components::Secdist>().Get();
  const auto& settings_multi =
      secdist.Get<storages::mysql::settings::MysqlSettingsMulti>();
  const auto& settings =
      settings_multi.Get(storages::mysql::settings::GetSecdistAlias(config));

  return std::make_shared<storages::mysql::Cluster>(resolver, settings, config);
}

}  // namespace

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()},
      cluster_{CreateCluster(dns_.GetResolver(), config, context)} {
  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
      "mysql", [this](utils::statistics::Writer& writer) {
        cluster_->WriteStatistics(writer);
      });
}

Component::~Component() { statistics_holder_.Unregister(); }

std::shared_ptr<storages::mysql::Cluster> Component::GetCluster() const {
  return cluster_;
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: MySQL client component
additionalProperties: false
properties:
    initial_pool_size:
        type: integer
        description: number of connections created initially
        defaultDescription: 1
    max_pool_size:
        type: integer
        description: maximum number of created connections
        defaultDescription: 10
)");
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
