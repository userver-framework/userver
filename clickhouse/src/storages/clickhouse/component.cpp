#include <userver/storages/clickhouse/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/statistics/writer.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/storages/clickhouse/cluster.hpp>

#include <storages/clickhouse/impl/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ClickHouse::ClickHouse(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()} {
  const auto& secdist = context.FindComponent<Secdist>().Get();
  const auto& settings_multi =
      secdist.Get<storages::clickhouse::impl::ClickhouseSettingsMulti>();
  const auto& settings =
      settings_multi.Get(storages::clickhouse::impl::GetSecdistAlias(config));

  cluster_ = std::make_shared<storages::clickhouse::Cluster>(dns_.GetResolver(),
                                                             settings, config);

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterWriter(
      "clickhouse",
      [this](utils::statistics::Writer& writer) {
        if (cluster_) {
          cluster_->WriteStatistics(writer);
        }
      },
      {{"clickhouse_database", settings.auth_settings.database}});
}

ClickHouse::~ClickHouse() { statistics_holder_.Unregister(); }

storages::clickhouse::ClusterPtr ClickHouse::GetCluster() const {
  return cluster_;
}

yaml_config::Schema ClickHouse::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: ClickHouse client component
additionalProperties: false
properties:
    secdist_alias:
        type: string
        description: name of the key in secdist config
    initial_pool_size:
        type: integer
        description: number of connections created initially
        defaultDescription: 5
    max_pool_size:
        type: integer
        description: maximum number of created connections
        defaultDescription: 10
    queue_timeout:
        type: string
        description: client waiting for a free connection time limit
        defaultDescription: 1s
    use_secure_connection:
        type: boolean
        description: whether to use TLS for connections
        defaultDescription: true
    compression:
        type: string
        description: compression method to use (none / lz4)
        defaultDescription: none
)");
}

}  // namespace components

USERVER_NAMESPACE_END
