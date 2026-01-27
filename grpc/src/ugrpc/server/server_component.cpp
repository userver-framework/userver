#include <userver/ugrpc/server/server_component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/logging/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <ugrpc/server/impl/parse_config.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/ugrpc/server/server_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServerComponent::ServerComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
    : ComponentBase(config, context),
      server_(
          impl::ParseServerConfig(config),
          context.FindComponent<components::StatisticsStorage>().GetStorage(),
          context.FindComponent<components::DynamicConfig>().GetSource()
      ),
      service_defaults_(std::make_unique<
                        impl::ServiceDefaults>(impl::ParseServiceDefaults(config["service-defaults"], context)))
{}

ServerComponent::~ServerComponent() { server_.Stop(); }

Server& ServerComponent::GetServer() noexcept { return server_; }

ServiceConfig ServerComponent::ParseServiceConfig(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
) {
    return impl::ParseServiceConfig(config, context, *service_defaults_);
}

void ServerComponent::OnAllComponentsLoaded() { server_.Start(); }

void ServerComponent::OnAllComponentsAreStopping() { server_.StopServing(); }

yaml_config::Schema ServerComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<ComponentBase>("src/ugrpc/server/server_component.yaml");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
