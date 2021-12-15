#include <userver/ugrpc/server/server_component.hpp>

#include <userver/components/component_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServerComponent::ServerComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      server_(config.As<ServerConfig>()) {}

Server& ServerComponent::GetServer() noexcept { return server_; }

void ServerComponent::OnAllComponentsLoaded() { server_.Start(); }

void ServerComponent::OnAllComponentsAreStopping() { server_.Stop(); }

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
