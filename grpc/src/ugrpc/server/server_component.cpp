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
  return yaml_config::MergeSchemas<LoggableComponentBase>(R"(
type: object
description: Component that configures and manages the gRPC server.
additionalProperties: false
properties:
    port:
        type: integer
        description: the port to use for all gRPC services, or 0 to pick any available
    channel-args:
        type: object
        description: a map of channel arguments, see gRPC Core docs
        defaultDescription: '{}'
        additionalProperties:
            type: string
            description: value of channel argument, must be string or integer
        properties: {}
    native-log-level:
        type: string
        description: min log level for the native gRPC library
        defaultDescription: 'error'
        enum:
          - trace
          - debug
          - info
          - warning
          - error
          - critical
          - none
    enable-channelz:
        type: boolean
        description: enable channelz
)");
}

}  // namespace ugrpc::server

USERVER_NAMESPACE_END
