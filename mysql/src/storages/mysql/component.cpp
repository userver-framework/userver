#include <userver/storages/mysql/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_context.hpp>

#include <storages/mysql/settings/settings.hpp>
#include <userver/storages/mysql/cluster.hpp>
#include <userver/storages/secdist/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

MySQL::MySQL(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()} {
  const auto& secdist = context.FindComponent<Secdist>().Get();
  const auto& settings_multi =
      secdist.Get<storages::mysql::settings::MysqlSettingsMulti>();
  const auto& settings =
      settings_multi.Get(storages::mysql::settings::GetSecdistAlias(config));

  cluster_ = std::make_shared<storages::mysql::Cluster>(dns_.GetResolver(),
                                                        settings, config);
}

MySQL::~MySQL() = default;

std::shared_ptr<storages::mysql::Cluster> MySQL::GetCluster() const {
  return cluster_;
}

}  // namespace components

USERVER_NAMESPACE_END
