#include <userver/storages/clickhouse/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/storages/secdist/component.hpp>

#include <userver/storages/clickhouse/cluster.hpp>
#include <userver/storages/clickhouse/settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ClickHouse::ClickHouse(const ComponentConfig& config,
                       const ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()} {
  const auto& secdist = context.FindComponent<Secdist>().Get();
  const auto& settings_multi =
      secdist.Get<storages::clickhouse::ClickhouseSettingsMulti>();
  const auto& settings =
      settings_multi.Get(storages::clickhouse::GetDbName(config));

  cluster_ = std::make_shared<storages::clickhouse::Cluster>(
      &dns_.GetResolver(), settings, config);

  auto& statistics_storage =
      context.FindComponent<components::StatisticsStorage>();
  statistics_holder_ = statistics_storage.GetStorage().RegisterExtender(
      "clickhouse." + config.Name(),
      [this](const auto&) { return cluster_->GetStatistics(); });
}

ClickHouse::~ClickHouse() { statistics_holder_.Unregister(); }

storages::clickhouse::ClusterPtr ClickHouse::GetCluster() const {
  return cluster_;
}

}  // namespace components

USERVER_NAMESPACE_END
