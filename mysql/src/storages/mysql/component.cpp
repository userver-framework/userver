#include <userver/storages/mysql/component.hpp>

#include <userver/clients/dns/component.hpp>
#include <userver/components/component_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

MySQL::MySQL(const ComponentConfig& config, const ComponentContext& context)
    : LoggableComponentBase{config, context},
      dns_{context.FindComponent<clients::dns::Component>()},
      cluster_{nullptr} {}

MySQL::~MySQL() = default;

std::shared_ptr<storages::mysql::Cluster> MySQL::GetCluster() const {
  return cluster_;
}

}  // namespace components

USERVER_NAMESPACE_END
