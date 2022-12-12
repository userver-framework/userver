#include <userver/storages/mysql/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_context.hpp>

#include <storages/mysql/settings/host_settings.hpp>
#include <userver/storages/mysql/cluster.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

namespace {

std::shared_ptr<storages::mysql::Cluster> BuildCluster(
    clients::dns::Resolver& resolver) {
  storages::mysql::settings::HostSettings settings{resolver, "localhost", 3306};

  std::vector<storages::mysql::settings::HostSettings> hosts{
      std::move(settings)};

  return std::make_shared<storages::mysql::Cluster>(std::move(hosts));
}

}  // namespace

MySQL::MySQL(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()},
      cluster_{BuildCluster(dns_.GetResolver())} {}

MySQL::~MySQL() = default;

std::shared_ptr<storages::mysql::Cluster> MySQL::GetCluster() const {
  return cluster_;
}

}  // namespace components

USERVER_NAMESPACE_END
