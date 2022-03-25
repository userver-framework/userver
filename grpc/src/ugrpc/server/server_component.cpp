#include <userver/ugrpc/server/server_component.hpp>

#include <userver/components/component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

ServerComponent::ServerComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      server_(
          config.As<ServerConfig>(),
          context.FindComponent<components::StatisticsStorage>().GetStorage()) {
}

Server& ServerComponent::GetServer() noexcept { return server_; }

void ServerComponent::OnAllComponentsLoaded() { server_.Start(); }

void ServerComponent::OnAllComponentsAreStopping() { server_.Stop(); }

yaml_config::Schema ServerComponent::GetStaticConfigSchema() {
  yaml_config::Schema schema(R"(
type: object
description: grpc-server config
additionalProperties: false
properties:
    port:
        type: integer
        description: the port to use for all gRPC services, or 0 to pick any available
    native-log-level:
        type: string
        description: min log level for the native gRPC library
        defaultDescription: 'error'
)");
  yaml_config::Merge(schema, LoggableComponentBase::GetStaticConfigSchema());
  return schema;
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
